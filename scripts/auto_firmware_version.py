import subprocess
import datetime

Import("env")

def get_firmware_specifier_build_flag():
    ret = subprocess.run(["git", "describe", "--tags", "--dirty"], stdout=subprocess.PIPE, text=True) #Uses any tags
    build_version = ret.stdout.strip()
    build_flag = "-D BUILD_GIT_VERSION=\\\"" + build_version + "\\\""
    print ("Build version: " + build_version)
    return (build_flag)

def get_time_specifier_build_flag():
    build_timestamp = datetime.datetime.now(datetime.UTC).strftime("%Y-%m-%dT%H:%M:%SZ")
    build_flag = "-D BUILD_TIMESTAMP=\\\"" + build_timestamp + "\\\""
    print ("Build date: " + build_timestamp)
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_firmware_specifier_build_flag(), get_time_specifier_build_flag()]
)
