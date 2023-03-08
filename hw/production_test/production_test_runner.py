#!/usr/bin/env python
from typing import Any
import production_tests

pass_msg = '''
  _____                _____    _____
 |  __ \\      /\\      / ____|  / ____|
 | |__) |    /  \\    | (___   | (___
 |  ___/    / /\\ \\    \\___ \\   \\___ \\
 | |       / ____ \\   ____) |  ____) |
 |_|      /_/    \\_\\ |_____/  |_____/
 '''

fail_msg = '''
  ______              _____   _
 |  ____|     /\\     |_   _| | |
 | |__       /  \\      | |   | |
 |  __|     / /\\ \\     | |   | |
 | |       / ____ \\   _| |_  | |____
 |_|      /_/    \\_\\ |_____| |______|
 '''

ANSI = {
    'fg_black': "\u001b[30m",
    'fg_red': "\u001b[31m",
    'fg_green': "\u001b[32m",
    'fg_yellow': "\u001b[33m",
    'fg_blue': "\u001b[34m",
    'fg_magenta': "\u001b[35m",
    'fg_cyan': "\u001b[36m",
    'fg_white': "\u001b[37m",
    'bg_black': "\u001b[40m",
    'bg_red': "\u001b[41m",
    'bg_green': "\u001b[42m",
    'bg_yellow': "\u001b[43m",
    'bg_blue': "\u001b[44m",
    'bg_magenta': "\u001b[45m",
    'bg_cyan': "\u001b[46m",
    'bg_white': "\u001b[47m",
    'reset': "\u001b[0m",
    'bold': "\u001b[1m",
    'underline': "\u001b[4m"
}


def run_tests(test_list: list[Any]) -> bool:
    try:
        for test in test_list:
            print("\n{:}Test step: {:}{:} ({:})".format(
                ANSI['bold'],
                test.__name__,
                ANSI['reset'],
                test.__doc__
            ))
            if not test():
                print(
                    'Failure at test step "{:}"'.format(
                        test.__name__))
                return False
    except Exception as e:
        print(
            'Error while running test step "{:}", exception:{:}'.format(
                test.__name__, str(e)))
        return False

    return True


if __name__ == '__main__':
    last_a = '1'

    # Allow any of the settings in the parameters structure to be
    # overridden
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-l',
        '--list',
        action='store_true',
        help='List available tests, thenexit')
    parser.add_argument(
        '-r',
        '--run_test',
        required=False,
        help='Run the specified test sequence or manual test, then exit')
    for setting, value in production_tests.parameters.items():
        parser.add_argument(
            '--' + setting,
            help='Default setting: ' + value)
    args = parser.parse_args()

    for arg in args.__dict__:
        if args.__dict__[arg] is not None:
            production_tests.parameters[arg] = args.__dict__[arg]

    if args.list:
        print("available tests:")
        for name, test_list in production_tests.test_sequences.items():
            print('{:}: {:}'.format(name, ', '.join(
                [test.__name__ for test in test_list])))
        for test in production_tests.manual_tests:
            print('{:}: {:}'.format(test.__name__, test.__doc__))
        exit(0)

    if args.run_test is not None:
        result = False
        found = False

        if args.run_test in production_tests.test_sequences:
            result = run_tests(
                production_tests.test_sequences[args.run_test])
            found = True
        else:
            for test in production_tests.manual_tests:
                if args.run_test == test.__name__:
                    result = run_tests([test])
                    found = True
                    break

        if not found:
            print('Test not found:{:}'.format(args.run_test))
            exit(1)

        if not result:
            production_tests.reset()
            exit(1)

        exit(0)

    print('\n\nProduction test system')

    while True:
        print('\n\n')

        options = []

        print('=== Test sequences ===')
        i = 1
        for name, test_list in production_tests.test_sequences.items():
            print('{:}{:}. {:}{:}: {:}'.format(ANSI['bold'], i, name, ANSI['reset'], ', '.join(
                [test.__name__ for test in test_list])))
            options.append(test_list)
            i += 1

        print('\n=== Manual tests ===')
        for test in production_tests.manual_tests:
            print(
                '{:}{:}. {:}{:}: {:}'.format(
                    ANSI['bold'],
                    i,
                    test.__name__,
                    ANSI['reset'],
                    test.__doc__))
            options.append([test])
            i += 1

        a = input(
            '\n\n\nPress return to run test {:}, or type in a new option number and press return:'.format(last_a))
        if a == '':
            a = last_a

        try:
            test_sequence = options[int(a) - 1]
        except IndexError as e:
            print('Invalid input')
            continue
        except ValueError as e:
            print('Invalid input')
            continue

        if not run_tests(test_sequence):
            print(ANSI['bg_red'] + fail_msg + ANSI['reset'])
            production_tests.reset()
        else:
            print(ANSI['bg_green'] + pass_msg + ANSI['reset'])

        last_a = a
