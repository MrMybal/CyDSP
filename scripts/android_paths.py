"""Resolve Android tools from local environment configuration."""
import os
from pathlib import Path
import sys


def android_sdk():
    configured = os.environ.get('ANDROID_SDK_ROOT') or os.environ.get('ANDROID_HOME')
    if configured:
        return Path(configured)
    if os.name == 'nt':
        local = os.environ.get('LOCALAPPDATA')
        return (Path(local) if local else Path.home() / 'AppData/Local') / 'Android/Sdk'
    if sys.platform == 'darwin':
        return Path.home() / 'Library/Android/sdk'
    return Path.home() / 'Android/Sdk'


def android_ndk():
    configured = os.environ.get('ANDROID_NDK_HOME') or os.environ.get('ANDROID_NDK_ROOT')
    return Path(configured) if configured else android_sdk() / 'ndk/27.2.12479018'


def android_adb():
    return android_sdk() / 'platform-tools' / ('adb.exe' if os.name == 'nt' else 'adb')
