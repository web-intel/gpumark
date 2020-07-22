#
# Copyright (c) 2019 The Aquarium Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

vars = {
  'chromium_git': 'https://chromium.googlesource.com',
  'github_git': 'https://github.com',
  'webatintel_git': 'https://github.com/webatintel',
  'build_revision': '896323eeda1bd1b01156b70625d5e14de225ebc3',
  'buildtools_revision': '74cfb57006f83cfe050817526db359d5c8a11628',
  'tools_clang_revision': '698732d5db36040c07d5cc5f9137fcc943494c11',
  'util_revision': '61f60539c3c62dd807029ab223bbb45766a6fe6a',
}

deps = {
  'build': {
    'url': '{chromium_git}/chromium/src/build@{build_revision}',
  },
  'buildtools': {
    'url': '{chromium_git}/chromium/src/buildtools@{buildtools_revision}',
  },
  'tools/clang': {
    'url': '{chromium_git}/chromium/src/tools/clang@{tools_clang_revision}',
  },
  'third_party/util': {
    'url': '{webatintel_git}/util@{util_revision}',
  },
  'third_party/stb': {
    #'url': '{github_git}/nothings/stb.git@c7110588a4d24c4bb5155c184fbb77dd90b3116e',
    'url': '{github_git}/nothings/stb.git@f54acd4e13430c5122cab4ca657705c84aa61b08',
  },
  'third_party/glfw': {
    'url': '{github_git}/glfw/glfw.git@2de2589f910b1a85905f425be4d32f33cec092df',
  },
  'third_party/rapidjson': {
    'url': '{github_git}/Tencent/rapidjson.git',
  },
}

hooks = [
  # Pull clang-format binaries using checked-in hashes.
  {
    'name': 'clang_format_win',
    'pattern': '.',
    'condition': 'host_os == "win"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/win/clang-format.exe.sha1',
    ],
  },
  {
    'name': 'clang_format_mac',
    'pattern': '.',
    'condition': 'host_os == "mac"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/mac/clang-format.sha1',
    ],
  },
  {
    'name': 'clang_format_linux',
    'pattern': '.',
    'condition': 'host_os == "linux"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/linux64/clang-format.sha1',
    ],
  },
  # Pull the compilers and system libraries for hermetic builds
  {
    'name': 'sysroot_x86',
    'pattern': '.',
    'condition': 'checkout_linux and ((checkout_x86 or checkout_x64))',
    'action': ['python', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x86'],
  },
  {
    'name': 'sysroot_x64',
    'pattern': '.',
    'condition': 'checkout_linux and (checkout_x64)',
    'action': ['python', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x64'],
  },
  {
    'name': 'clang',
    'pattern': '.',
    'action': ['python', 'tools/clang/scripts/update.py'],
  },
  # Pull rc binaries using checked-in hashes.
  {
    'name': 'rc_win',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "win"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-browser-clang/rc',
                '-s', 'build/toolchain/win/rc/win/rc.exe.sha1',
    ],
  },
  {
    'name': 'rc_mac',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "mac"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-browser-clang/rc',
                '-s', 'build/toolchain/win/rc/mac/rc.sha1',
    ],
  },
  {
    'name': 'rc_linux',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "linux"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-browser-clang/rc',
                '-s', 'build/toolchain/win/rc/linux64/rc.sha1',
    ],
  },
]

recursedeps = [
  # buildtools provides clang_format, libc++, and libc++abi
  'buildtools',
]
