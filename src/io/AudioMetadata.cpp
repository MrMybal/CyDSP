#include "cydsp/io/AudioMetadata.h"
#include <choc/text/choc_JSON.h>
#include <choc/memory/choc_Base64.h>
#include <algorithm>
#include <stdexcept>
namespace cydsp {
namespace {
using Value=choc::value::Value;
using View=choc::value::ValueView;
using Bytes=std::vector<std::uint8_t>;
constexpr std::size_t limit=33554432;
void check(bool v){if(!v)throw std::invalid_argument("audio metadata");}
Value json(std::string_view text) {
    check(text.size()<=limit);
    unsigned depth=0;bool string=false,escape=false;
    for(char c:text) {
        if(string){if(escape)escape=false;else if(c=='\\')escape=true;else if(c=='"')string=false;}
        else if(c=='"')string=true;
        else if(c=='{'||c=='['){check(++depth<=32);}
        else if(c=='}'||c==']'){check(depth!=0);--depth;}
    }
    check(!string&&depth==0);
    return text.empty()?choc::value::createEmptyArray():choc::json::parse(text);
}
std::uint64_t number(View v,std::uint64_t maximum=UINT32_MAX) {
    check(v.isInt());const auto n=v.get<std::int64_t>();check(n>=0&&std::uint64_t(n)<=maximum);return std::uint64_t(n);
}
std::uint64_t field(View v,const char* key,std::uint64_t fallback=0,std::uint64_t max=UINT32_MAX) {
    auto f=v[key];return f.isVoid()?fallback:number(f,max);
}
std::string text(View v,std::size_t maximum=1048576) {check(v.isString());auto s=v.getString();check(s.size()<=maximum);return std::string(s);}
std::string fieldText(View v,const char* key){return v[key].isVoid()?std::string{}:text(v[key]);}
Bytes decode(View v) {
    auto s=text(v,22369624);Bytes out;
    check(choc::base64::decodeToContainer(out,s));check(out.size()<=16777216);return out;
}
std::uint32_t read(std::span<const std::uint8_t> b,std::size_t at,unsigned n=4) {
    check(at<=b.size()&&n<=b.size()-at);std::uint32_t v=0;
    for(unsigned i=0;i<n;++i)v|=std::uint32_t(b[at+i])<<(8*i);return v;
}
void put(Bytes& b,std::uint64_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b.push_back(std::uint8_t(v>>(8*i)));}
std::string four(std::uint32_t v){std::string s(4,' ');for(unsigned i=0;i<4;++i)s[i]=char(v>>(8*i));return s;}
std::uint32_t four(std::string_view v){check(v.size()==4);std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(static_cast<unsigned char>(v[i]))<<(8*i);return n;}
std::string terminated(std::span<const std::uint8_t> b,std::size_t at) {
    check(at<=b.size());auto end=at;while(end<b.size()&&b[end])++end;
    check(end-at<=1048576);return {reinterpret_cast<const char*>(b.data()+at),end-at};
}
template<class F>void children(std::span<const std::uint8_t> bytes,F f) {
    for(std::size_t at=0;at<bytes.size();) {
        check(bytes.size()-at>=8);const auto tag=read(bytes,at),size=read(bytes,at+4);at+=8;
        check(size<=bytes.size()-at&&(size&1u)<=bytes.size()-at-size);
        f(tag,bytes.subspan(at,size),bytes.subspan(at-8,8+size+(size&1u)));at+=size+(size&1u);
    }
}
void child(Bytes& b,std::uint32_t tag,const Bytes& payload) {
    check(payload.size()<=16777216&&b.size()<=16777216-payload.size());
    put(b,tag);put(b,payload.size());b.insert(b.end(),payload.begin(),payload.end());if(payload.size()&1)b.push_back(0);
}
Value rawList(std::string_view subtype,const Bytes& bytes) {
    return choc::json::create("type","LIST","listType",subtype,"data",choc::base64::encodeToString(bytes));
}
std::string canonical(std::string_view key) {
    for(auto pair:{std::pair{"INAM","title"},{"IART","artist"},{"IPRD","album"},{"ICMT","comment"},
                   {"ICOP","copyright"},{"IGNR","genre"},{"ICRD","date"},{"ISFT","software"}})
        if(key==pair.first)return pair.second;
    return std::string(key);
}
std::string nativeKey(std::string_view key) {
    for(auto tag:{"INAM","IART","IPRD","ICMT","ICOP","IGNR","ICRD","ISFT"})
        if(canonical(tag)==key)return tag;
    check(key.size()==4);return std::string(key);
}
bool valid(const AudioMetadata& m) {
    if(m.version!=1||m.container!="WAV"||m.markers.size()>65536||m.regions.size()>65536||
       m.loops.size()>65536||m.tags.size()>65536||m.unityNote>127||m.preservedNative.size()>limit)return false;
    auto good=[](const std::string& s){return s.size()<=1048576&&s.find('\0')==std::string::npos;};
    for(std::size_t i=0;i<m.markers.size();++i){
        auto& v=m.markers[i];if(v.frame>UINT32_MAX||!good(v.name)||!good(v.note))return false;
        for(std::size_t j=0;j<i;++j)if(m.markers[j].id==v.id)return false;
    }
    for(auto& v:m.regions)if(v.lengthFrames>UINT32_MAX||!good(v.text))return false;
    for(auto& v:m.loops)if(v.startFrame>=v.endFrame||v.startFrame>UINT32_MAX||v.endFrame>UINT64_C(0x100000000)||
        unsigned(v.mode)>2)return false;
    for(auto& v:m.tags)if(!good(v.key)||!good(v.value))return false;
    return true;
}
void lists(View root,AudioMetadata& m) {
    for(auto chunk:root)if(fieldText(chunk,"type")=="LIST") {
        const auto subtype=fieldText(chunk,"listType");
        if(chunk["data"].isString()) {
            const auto bytes=decode(chunk["data"]);
            children(bytes,[&](auto tag,auto payload,auto) {
                if((subtype=="INFO"||subtype=="info")&&canonical(four(tag))!=four(tag))m.tags.push_back({canonical(four(tag)),terminated(payload,0)});
                if(subtype!="adtl")return;
                if(tag==0x6c62616c||tag==0x65746f6e) { // labl/note
                    check(payload.size()>=4);const auto id=read(payload,0);
                    auto i=std::find_if(m.markers.begin(),m.markers.end(),[&](const auto& v){return v.id==id;});
                    if(i!=m.markers.end())(tag==0x6c62616c?i->name:i->note)=terminated(payload,4);
                } else if(tag==0x7478746c) { // ltxt
                    check(payload.size()>=20);
                    m.regions.push_back({read(payload,0),read(payload,4),read(payload,8),
                        std::uint16_t(read(payload,12,2)),std::uint16_t(read(payload,14,2)),
                        std::uint16_t(read(payload,16,2)),std::uint16_t(read(payload,18,2)),terminated(payload,20)});
                }
            });
        } else if((subtype.empty()||subtype=="INFO"||subtype=="info")&&chunk["items"].isArray()) {
            for(auto item:chunk["items"])m.tags.push_back({canonical(fieldText(item,"type")),fieldText(item,"value")});
        }
    }
}
Value native(const AudioMetadata& m,std::string_view fallback) {
    check(valid(m));auto old=json(m.preservedNative.empty()?fallback:m.preservedNative);check(old.isArray());
    auto out=choc::value::createEmptyArray();Value smpl=choc::value::createObject({});
    for(auto chunk:old) {
        const auto type=fieldText(chunk,"type");
        if(type=="cue ")continue;
        if(type=="smpl"){smpl=Value(chunk);continue;}
        if(type=="LIST") {
            const auto subtype=fieldText(chunk,"listType");
            if(subtype=="adtl"&&chunk["data"].isString()) {
                Bytes kept;auto bytes=decode(chunk["data"]);
                children(bytes,[&](auto tag,auto,auto whole) {
                    if(tag!=0x6c62616c&&tag!=0x65746f6e&&tag!=0x7478746c)kept.insert(kept.end(),whole.begin(),whole.end());
                });
                if(!kept.empty())out.addArrayElement(rawList(subtype,kept));continue;
            }
            if((subtype=="INFO"||subtype=="info")&&chunk["data"].isString()) {
                Bytes kept;auto bytes=decode(chunk["data"]);
                children(bytes,[&](auto tag,auto,auto whole) {
                    if(canonical(four(tag))==four(tag))kept.insert(kept.end(),whole.begin(),whole.end());
                });
                if(!kept.empty())out.addArrayElement(rawList(subtype,kept));continue;
            }
            if(subtype.empty()&&chunk["items"].isArray())continue;
        }
        out.addArrayElement(Value(chunk));
    }
    auto cues=choc::value::createEmptyArray();Bytes adtl,info;
    for(auto& v:m.markers) {
        cues.addArrayElement(choc::json::create("ID",std::int64_t(v.id),"position",std::int64_t(v.position),
            "dataChunkID",std::int64_t(v.dataChunkId),"chunkStart",std::int64_t(v.chunkStart),
            "blockStart",std::int64_t(v.blockStart),"sampleStart",std::int64_t(v.frame)));
        for(bool note:{false,true}) {
            const auto& s=note?v.note:v.name;if(s.empty())continue;
            Bytes b;put(b,v.id);b.insert(b.end(),s.begin(),s.end());b.push_back(0);
            child(adtl,note?0x65746f6e:0x6c62616c,b);
        }
    }
    if(!m.markers.empty())out.addArrayElement(choc::json::create("type","cue ","cues",cues));
    for(auto& v:m.regions) {
        Bytes b;put(b,v.markerId);put(b,v.lengthFrames);put(b,v.purpose);
        put(b,v.country,2);put(b,v.language,2);put(b,v.dialect,2);put(b,v.codePage,2);
        b.insert(b.end(),v.text.begin(),v.text.end());b.push_back(0);child(adtl,0x7478746c,b);
    }
    if(!adtl.empty())out.addArrayElement(rawList("adtl",adtl));
    auto loops=choc::value::createEmptyArray();
    for(auto& v:m.loops)loops.addArrayElement(choc::json::create("ID",std::int64_t(v.id),"loopType",std::int64_t(v.mode),
        "startByte",std::int64_t(v.startFrame),"endByte",std::int64_t(v.endFrame-1),
        "fractionalOffset",std::int64_t(v.fractionalOffset),"playCount",std::int64_t(v.playCount)));
    if(!m.loops.empty()||smpl.hasObjectMember("type")) {
        smpl.setMember("type","smpl");smpl.setMember("loops",loops);
        smpl.setMember("midiUnityNote",std::int64_t(m.unityNote));smpl.setMember("midiPitchFraction",std::int64_t(m.pitchFraction));
        out.addArrayElement(smpl);
    }
    for(auto& v:m.tags) {
        auto key=nativeKey(v.key);Bytes b(v.value.begin(),v.value.end());b.push_back(0);child(info,four(key),b);
    }
    if(!info.empty())out.addArrayElement(rawList("INFO",info));
    return out;
}
}
Status getAudioMetadata(const AudioFileInfo& file,AudioMetadata& result)noexcept {
    try {
        AudioMetadata next;next.container=file.formatName;next.preservedNative=file.metadataJson;
        if(file.metadataJson.empty()||file.metadataJson=="null"||file.metadataJson=="[]") {
            result=std::move(next);return Status::ok;
        }
        if(file.formatName!="WAV")return Status::unsupported_format;
        auto root=json(file.metadataJson);check(root.isArray()&&root.size()<=65536);
        for(auto c:root) {
            const auto type=fieldText(c,"type");
            if(type=="cue "){
                check(c["cues"].isArray()&&c["cues"].size()<=65536);
                for(auto v:c["cues"])next.markers.push_back({std::uint32_t(field(v,"ID")),field(v,"sampleStart"),{},{},
                    std::uint32_t(field(v,"position")),std::uint32_t(field(v,"dataChunkID",0x61746164)),
                    std::uint32_t(field(v,"chunkStart")),std::uint32_t(field(v,"blockStart"))});
            } else if(type=="smpl") {
                next.unityNote=std::uint32_t(field(c,"midiUnityNote",60));next.pitchFraction=std::uint32_t(field(c,"midiPitchFraction"));
                check(c["loops"].isArray()&&c["loops"].size()<=65536);
                for(auto v:c["loops"])next.loops.push_back({std::uint32_t(field(v,"ID")),field(v,"startByte"),field(v,"endByte")+1,
                    AudioLoopMode(field(v,"loopType")),std::uint32_t(field(v,"fractionalOffset")),std::uint32_t(field(v,"playCount"))});
            }
        }
        lists(root,next);check(valid(next));result=std::move(next);return Status::ok;
    }catch(const std::bad_alloc&){return Status::allocation_failed;}catch(...){return Status::invalid_argument;}
}
Status setAudioMetadata(AudioFileInfo& file,const AudioMetadata& m)noexcept {
    if(file.formatName!="WAV"&&!file.formatName.empty())return Status::unsupported_format;
    try {auto encoded=choc::json::toString(native(m,file.metadataJson));check(encoded.size()<=limit);
        file.metadataJson=std::move(encoded);return Status::ok;
    }catch(const std::bad_alloc&){return Status::allocation_failed;}catch(...){return Status::invalid_argument;}
}
Status serializeAudioMetadata(const AudioMetadata& m,std::string& output)noexcept {
    try {
        auto chunks=native(m,{});
        auto root=choc::json::create("schema","CyDSP.AudioMetadata","version",1,"container","WAV","native",chunks);
        auto encoded=choc::json::toString(root);check(encoded.size()<=limit);output=std::move(encoded);return Status::ok;
    }catch(const std::bad_alloc&){return Status::allocation_failed;}catch(...){return Status::invalid_argument;}
}
Status parseAudioMetadata(std::string_view input,AudioMetadata& output)noexcept {
    try {
        auto root=json(input);check(root.isObject()&&fieldText(root,"schema")=="CyDSP.AudioMetadata"&&
            field(root,"version")==1&&fieldText(root,"container")=="WAV"&&root["native"].isArray());
        AudioFileInfo file;file.formatName="WAV";file.metadataJson=choc::json::toString(root["native"]);
        return getAudioMetadata(file,output);
    }catch(const std::bad_alloc&){return Status::allocation_failed;}catch(...){return Status::invalid_argument;}
}
} // namespace cydsp
