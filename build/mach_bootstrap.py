#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import print_function, unicode_literals

import os
import platform
import sys

# TODO Bug 794506 Integrate with the in-tree virtualenv configuration.
SEARCH_PATHS = [
    'python/mach',
    'python/mozboot',
    'python/mozbuild',
    'build/pymake',
    'python/blessings',
    'python/psutil',
    'python/which',
    'other-licenses/ply',
    'xpcom/idl-parser',
    'testing',
    'testing/xpcshell',
    'testing/mozbase/mozprocess',
    'testing/mozbase/mozfile',
    'testing/mozbase/mozinfo',
]

# Individual files providing mach commands.
MACH_MODULES = [
    'addon-sdk/mach_commands.py',
    'layout/tools/reftest/mach_commands.py',
    'python/mach/mach/commands/commandinfo.py',
    'python/mozboot/mozboot/mach_commands.py',
    'python/mozbuild/mozbuild/config.py',
    'python/mozbuild/mozbuild/mach_commands.py',
    'python/mozbuild/mozbuild/frontend/mach_commands.py',
    'testing/mochitest/mach_commands.py',
    'testing/xpcshell/mach_commands.py',
]

def bootstrap(topsrcdir):
    # Ensure we are running Python 2.7+. We put this check here so we generate a
    # user-friendly error message rather than a cryptic stack trace on module
    # import.
    if sys.version_info[0] != 2 or sys.version_info[1] < 7:
        print('Python 2.7 or above (but not Python 3) is required to run mach.')
        print('You are running Python', platform.python_version())
        sys.exit(1)

    try:
        import mach.main
    except ImportError:
        sys.path[0:0] = [os.path.join(topsrcdir, path) for path in SEARCH_PATHS]
        import mach.main

    mach = mach.main.Mach(topsrcdir)
    for path in MACH_MODULES:
        mach.load_commands_from_file(os.path.join(topsrcdir, path))
    return mach
