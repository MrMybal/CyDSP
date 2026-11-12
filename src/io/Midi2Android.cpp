#define __ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__ 1
#include "detail/Midi2Backend.h"
#include <amidi/AMidi.h>
#include <android/api-level.h>
#include <cstring>
#include <dlfcn.h>
namespace cydsp::detail {
namespace {
JNIEnv* environment(JavaVM* vm, bool& attached) noexcept {
    void* pointer = nullptr;
    const auto result = vm->GetEnv(&pointer, JNI_VERSION_1_6);
    if (result == JNI_OK)
        return static_cast<JNIEnv*>(pointer);
    if (result != JNI_EDETACHED)
        return nullptr;
    JNIEnv* env = nullptr;
    if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
        return nullptr;
    attached = true;
    return env;
}
struct JavaDevice {
    JavaVM* vm = nullptr;
    jobject object = nullptr;
    ~JavaDevice() {
        if (!object || !vm)
            return;
        bool attached = false;
        if (auto* env = environment(vm, attached))
            env->DeleteGlobalRef(object);
        if (attached)
            vm->DetachCurrentThread();
    }
};
template <class T> T symbol(void* library, const char* name) noexcept {
    auto* raw = dlsym(library, name);
    T function{};
    static_assert(sizeof(function) == sizeof(raw));
    std::memcpy(&function, &raw, sizeof(function));
    return function;
}
struct AndroidMidi final : NativeMidi2 {
    std::shared_ptr<JavaDevice> java;
    void* library = nullptr;
    AMidiDevice* device = nullptr;
    AMidiOutputPort* input = nullptr;
    AMidiInputPort* output = nullptr;
    decltype(&AMidiDevice_fromJava) fromJava{};
    decltype(&AMidiDevice_release) release{};
    decltype(&AMidiDevice_getDefaultProtocol) protocol{};
    decltype(&AMidiOutputPort_open) openInput{};
    decltype(&AMidiOutputPort_close) closeInput{};
    decltype(&AMidiInputPort_open) openOutput{};
    decltype(&AMidiInputPort_close) closeOutput{};
    decltype(&AMidiOutputPort_receive) read{};
    decltype(&AMidiInputPort_sendWithTimestamp) write{};
    ReceiveMidi2 receive;
    void* context;
    std::size_t sent = 0, have = 0;
    std::uint32_t word = 0;
    AndroidMidi(ReceiveMidi2 f, void* c) : receive(f), context(c) {}
    ~AndroidMidi() override {
        if (input)
            closeInput(input);
        if (output)
            closeOutput(output);
        if (device)
            (void)release(device);
        if (library)
            dlclose(library);
    }
    Status initialize(const Midi2NativeRequest& r) noexcept {
        if (android_get_device_api_level() < 33)
            return Status::unsupported_backend;
        java = std::static_pointer_cast<JavaDevice>(r.android);
        if (!java)
            return Status::invalid_argument;
        library = dlopen("libamidi.so", RTLD_NOW | RTLD_LOCAL);
        if (!library)
            return Status::unsupported_backend;
#define CYDSP_AMIDI(member, name)                                                                  \
    member = symbol<decltype(member)>(library, name);                                              \
    if (!member)                                                                                   \
    return Status::unsupported_backend
        CYDSP_AMIDI(fromJava, "AMidiDevice_fromJava");
        CYDSP_AMIDI(release, "AMidiDevice_release");
        CYDSP_AMIDI(protocol, "AMidiDevice_getDefaultProtocol");
        CYDSP_AMIDI(openInput, "AMidiOutputPort_open");
        CYDSP_AMIDI(closeInput, "AMidiOutputPort_close");
        CYDSP_AMIDI(openOutput, "AMidiInputPort_open");
        CYDSP_AMIDI(closeOutput, "AMidiInputPort_close");
        CYDSP_AMIDI(read, "AMidiOutputPort_receive");
        CYDSP_AMIDI(write, "AMidiInputPort_sendWithTimestamp");
#undef CYDSP_AMIDI
        bool attached = false;
        auto* env = environment(java->vm, attached);
        if (!env)
            return Status::device_unavailable;
        const auto result = fromJava(env, java->object, &device);
        if (attached)
            java->vm->DetachCurrentThread();
        if (result != AMEDIA_OK || !device)
            return Status::device_unavailable;
        const auto selected = static_cast<int>(protocol(device));
        if (selected < 0 || (selected > 4 && selected != 17 && selected != 18))
            return Status::unsupported_format;
        if (r.receivePort >= 0 && openInput(device, r.receivePort, &input) != AMEDIA_OK)
            return Status::device_unavailable;
        if (r.sendPort >= 0 && openOutput(device, r.sendPort, &output) != AMEDIA_OK)
            return Status::device_unavailable;
        return Status::ok;
    }
    Status send(const Midi2TimedPacket& p) noexcept override {
        if (!output)
            return Status::unsupported_format;
        std::array<std::uint8_t, 16> bytes{};
        std::size_t size = 0;
        auto status = p.packet.toBytes(bytes, size);
        if (status != Status::ok)
            return status;
        const auto count = write(output, bytes.data() + sent, size - sent,
                                 static_cast<std::int64_t>(p.timestampNs));
        if (count < 0)
            return Status::io_error;
        sent += static_cast<std::size_t>(count);
        if (sent < size)
            return Status::would_block;
        sent = 0;
        return Status::ok;
    }
    Status poll() noexcept override {
        if (!input)
            return Status::ok;
        std::array<std::uint8_t, 4096> bytes{};
        std::size_t size = 0;
        std::int32_t opcode = 0;
        std::int64_t timestamp = 0;
        const auto messages = read(input, &opcode, bytes.data(), bytes.size(), &size, &timestamp);
        if (messages < 0)
            return Status::io_error;
        if (!messages)
            return Status::would_block;
        if (opcode == AMIDI_OPCODE_FLUSH) {
            have = 0;
            word = 0;
            receive(context, {}, 0);
            return Status::ok;
        }
        if (opcode != AMIDI_OPCODE_DATA || size > bytes.size())
            return Status::invalid_argument;
        for (std::size_t i = 0; i < size; ++i) {
            word = (word << 8) | bytes[i];
            if (++have == 4) {
                receive(context, {&word, 1},
                        timestamp > 0 ? static_cast<std::uint64_t>(timestamp)
                                      : midi2TimestampNow());
                have = 0;
                word = 0;
            }
        }
        return Status::ok;
    }
    bool schedules() const noexcept override {
        return true;
    }
};
} // namespace
Status prepareAndroidMidi2(void* environmentPointer, void* device, int receive, int send,
                           Midi2NativeRequest& out) noexcept {
    if (!environmentPointer || !device || receive < -1 || send < -1 || (receive < 0 && send < 0))
        return Status::invalid_argument;
    if (android_get_device_api_level() < 33)
        return Status::unsupported_backend;
    try {
        auto next = std::make_shared<JavaDevice>();
        auto* env = static_cast<JNIEnv*>(environmentPointer);
        if (env->GetJavaVM(&next->vm) != JNI_OK)
            return Status::device_unavailable;
        next->object = env->NewGlobalRef(static_cast<jobject>(device));
        if (!next->object)
            return Status::allocation_failed;
        out.endpoint = {"amidi", "Android native UMP", Midi2Backend::androidAmidi, receive >= 0,
                        send >= 0};
        out.receivePort = receive;
        out.sendPort = send;
        out.android = std::move(next);
        return Status::ok;
    } catch (...) {
        return Status::allocation_failed;
    }
}
Status enumerateNativeMidi2(std::vector<Midi2EndpointInfo>& result) noexcept {
    (void)result;
    return Status::unsupported_backend;
}
Status createNativeMidi2(const Midi2NativeRequest& r, ReceiveMidi2 receive, void* context,
                         std::unique_ptr<NativeMidi2>& out) noexcept {
    if (r.endpoint.backend != Midi2Backend::androidAmidi)
        return Status::unsupported_backend;
    try {
        auto next = std::make_unique<AndroidMidi>(receive, context);
        const auto status = next->initialize(r);
        if (status == Status::ok)
            out = std::move(next);
        return status;
    } catch (...) {
        return Status::allocation_failed;
    }
}
} // namespace cydsp::detail
