import sys
sys.path.insert(1, 'third_party')
from util.base import *

class GPUMark():
    TEST_TARGETS = [
        'aquarium',
        'asteroid',
        'd3d11_compute',
        'd3d12_compute',
        'nbody',
        'vp_overlay',
    ]

    def __init__(self):
        self._parse_args()
        self._handle_ops()

    def sync(self):
        self.program.execute('git pull')
        self.program.execute_gclient(cmd_type='sync')
        self.program.execute_gclient(cmd_type='runhooks')
        Util.chdir(self.root_dir + '/build/util')
        self.program.execute('python lastchange.py -o LASTCHANGE')

    def makefile(self):
        gn_args = 'is_component_build=false'

        if self.build_type == 'debug':
            gn_args += ' is_debug=true'
        else:
            gn_args += ' is_debug=false'

        quotation = Util.get_quotation()
        cmd = 'gn --args=%s%s%s gen %s' % (quotation, gn_args, quotation, self.out_dir)
        if self.args.makefile_vs:
            cmd += ' --ide=vs2019'
        result = self.program.execute(cmd)

    def build(self):
        cmd = 'ninja -k1 -j%s -C %s %s' % (Util.CPU_COUNT, self.out_dir, self.build_target)
        self.program.execute(cmd, show_duration=True)

    def backup(self):
        date = self.program.execute('git log -1 --date=format:"%Y%m%d" --format="%ad"', return_out=True, show_cmd=False)[1].rstrip('\n').rstrip('\r')
        hash1 = self.program.execute('git rev-parse --short HEAD', return_out=True, show_cmd=False)[1].rstrip('\n').rstrip('\r')
        rev = '%s-%s' % (date, hash1)
        self.rev = rev
        Util.info('Begin to backup rev %s' % rev)
        self.backup_dir = '%s/backup/%s' % (self.program.root_dir, rev)
        targets = self.args.backup_target.split(',')
        Util.backup_gn_target(self.program.root_dir, self.out_dir, self.backup_dir, targets=targets, need_symbol=self.program.args.backup_symbol)

    def test(self):
        self._get_info()
        test_target_str = self.program.args.test_target
        if test_target_str == 'default':
            for target in self.TEST_TARGETS:
                self.test_target(target)
            return

        if test_target_str[0] == '-':
            is_positive_target = False
            test_target_str = test_target_str[1:]
        else:
            is_positive_target = True

        test_targets = test_target_str.split(':')

        for target in self.TEST_TARGETS:
            if test_targets:
                for test_target in test_targets:
                    if re.match(test_target, target):
                        match = (True == is_positive_target)
                        break
                else:
                    match = (False == is_positive_target)

            if match:
                self.test_target(target)

    def test_target(self, target):
        if target == 'aquarium':
            msaa_counts = [1, 4, 8]
            for msaa_count in msaa_counts:
                option = '--test-time 10 --msaa-count %s' % msaa_count
                self.test_target_option(target, option)
                option += ' --disable-d3d12-render-pass'
                self.test_target_option(target, option)
        elif target == 'asteroid':
            option = '--close-after 10'
            self.test_target_option(target, option)
        else:
            option = ''
            self.test_target_option(target, option)

    def test_target_option(self, target, option):
        lines = Util.execute('%s\%s\%s %s' % (self.program.root_dir, self.out_dir, target, option), return_out=True, exit_on_error=False)[1].split('\n')
        for line in lines:
            match = re.search('\[RESULT\] (.*)', line)
            if match:
                print(match.group(1))


    def release(self):
        self.sync()
        self.makefile()
        self.build()
        self.test()

    def _get_info(self):
        alias_property_dict = {
            'CPU': ['CPU', 'MaxClockSpeed,Name,NumberOfEnabledCore,NumberOfLogicalProcessors'],
            'GPU': ['path win32_VideoController', 'CurrentHorizontalResolution,CurrentVerticalResolution,DriverVersion,Name'],
            'SYSTEM': ['ComputerSystem', 'TotalPhysicalMemory'],
            'OS': ['OS', 'OSArchitecture,Version'],
        }
        self.info = {}

        for key,value in alias_property_dict.items():
            cmd = 'wmic %s get %s /value' % (value[0], value[1])
            lines = self.program.execute(cmd, show_cmd=False, return_out=True)[1].split('\r\r\n')
            self.info[key] = {}
            for line in lines:
                match = re.match('(.*)=(.*)', line)
                if match:
                    self.info[key][match.group(1)] = match.group(2)
                    # Skip the second
                    #if match.group(1) == value[1].split(',')[-1]:
                    #    break

        print(self.info)

    def _parse_args(self):
        parser = argparse.ArgumentParser(description='Script for GPUMark',
                                        formatter_class=argparse.RawTextHelpFormatter,
                                        epilog='''
examples:
python %(prog)s --sync --makefile --build --build-target msaa --test --test-target="-abc:def"
python %(prog)s --build --build-target overlay --test --test-target="overlay"
        ''')

        parser.add_argument('--is-debug', dest='is_debug', help='is debug', action='store_true')
        parser.add_argument('--symbol-level', dest='symbol_level', help='symbol level', type=int, default=0)
        parser.add_argument('--sync', dest='sync', help='sync', action='store_true')
        parser.add_argument('--makefile', dest='makefile', help='makefile', action='store_true')
        parser.add_argument('--makefile-vs', dest='makefile_vs', help='Produce Visual Studio project and solution files', action='store_true')
        parser.add_argument('--build', dest='build', help='build', action='store_true')
        parser.add_argument('--build-target', dest='build_target', help='build target', default='default')
        parser.add_argument('--test', dest='test', help='test', action='store_true')
        parser.add_argument('--test-target', dest='test_target', help='test target with same rule as --gtest_filter', default='default')
        parser.add_argument('--backup', dest='backup', help='backup', action='store_true')
        parser.add_argument('--backup-target', dest='backup_target', help='backup target')
        parser.add_argument('--backup-symbol', dest='backup_symbol', help='backup symbol', action='store_true')
        parser.add_argument('--release', dest='release', help='release', action='store_true')


        self.program = Program(parser)
        args = self.program.args

        if args.is_debug:
            self.build_type = 'debug'
        else:
            self.build_type = 'release'
        self.build_type_cap = self.build_type.capitalize()
        self.out_dir = 'out/%s' % self.build_type_cap
        if args.build_target == 'default':
            self.build_target = ''
        else:
            self.build_target = args.build_target

        self.root_dir = self.program.root_dir
        self.args = args

    def _handle_ops(self):
        args = self.program.args
        if args.sync:
            self.sync()
        if args.makefile:
            self.makefile()
        if args.build:
            self.build()
        if args.backup:
            self.backup()
        if args.test:
            self.test()
        if args.release:
            self.release()

if __name__ == '__main__':
    GPUMark()
