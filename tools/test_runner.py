# Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest
import os
import sys
import paddle
import paddle.fluid as fluid
import importlib
import paddle.fluid.core as core
from io import StringIO

sys.path.append(os.path.abspath(os.path.dirname(__file__)))
import static_mode_white_list


def main():
    sys.path.append(os.getcwd())
    if core.is_compiled_with_cuda() or core.is_compiled_with_rocm():
        if os.getenv('FLAGS_enable_gpu_memory_usage_log') == None:
            os.environ['FLAGS_enable_gpu_memory_usage_log'] = 'true'
            os.environ['FLAGS_enable_gpu_memory_usage_log_mb'] = 'false'

    some_test_failed = False
    for module_name in sys.argv[1:]:
        flag_need_static_mode = False
        if module_name in static_mode_white_list.STATIC_MODE_TESTING_LIST:
            flag_need_static_mode = True
            paddle.enable_static()
        buffer = StringIO()
        main = fluid.Program()
        startup = fluid.Program()
        scope = fluid.core.Scope()
        with fluid.program_guard(main, startup):
            with fluid.scope_guard(scope):
                with fluid.unique_name.guard():
                    test_loader = unittest.TestLoader()
                    module = importlib.import_module(module_name)
                    tests = test_loader.loadTestsFromModule(module)
                    res = unittest.TextTestRunner(stream=buffer).run(tests)

                    if not res.wasSuccessful():
                        some_test_failed = True
                        print(
                            module_name,
                            'failed\n',
                            buffer.getvalue(),
                            file=sys.stderr,
                        )
        if flag_need_static_mode:
            paddle.disable_static()

    if some_test_failed:
        exit(1)


if __name__ == '__main__':
    main()
