#!/usr/bin/env python
import hid_test

def import_pins(pin_filename):
    pins = {}
    with open(pin_filename + '.csv','r') as f:
        for line in f.readlines():
            [gpio,name] = line.strip().split(',')
            pins[name] = int(gpio)
    return pins

def read_pin_states(pins):
    d = hid_test.ice40_flasher()

    # Set all pins as inputs, pull-down
    for [name,gpio] in pins.items():
        d.gpio_set_direction(gpio,False)
        d.gpio_set_pulls(gpio,False,True)

    # One by one, set each pin as an output, read the states of all pins, then set the pin as an input again.
    changes = {}
    for [name,gpio] in pins.items():
        pre = d.gpio_get_all()
        d.gpio_set_direction(gpio,True)
        d.gpio_put(gpio,True)
        mid = d.gpio_get_all()
    
        d.gpio_set_direction(gpio,False)
        post = d.gpio_get_all()
    
        if (pre != 0) or (post != 0):
            print('Error in pre/post condition, pin:{:} pre:{:08x} post:{:08x}'.format(name,pre,post))
    
        change = []
        for [n,g] in pins.items():
            if (1<<g) & mid:
                change.append(n)
    
        changes[name] = change

    # Set all pins as inputs with no pulls
    for [name,gpio] in pins.items():
        d.gpio_set_direction(gpio,False)
        d.gpio_set_pulls(gpio,False,False)

    return changes

def read_pin_changes(pin_filename):
    changes_good = {}
    with open(pin_filename + '-good.csv','r') as f:
        for line in f.readlines():
            [name,change] = line.strip().split(',')

            # Handle empty change lists
            if change == '':
                changes_good[name] = []
            else:
                changes_good[name] = change.split(' ')
    return changes_good

def save_pin_changes(pin_filename, changes):
    with open(pin_filename + '-good.csv','w') as f:
        for [pin, change] in changes.items():
             f.write('{:},{:}\n'.format(pin, ' '.join([i for i in change])))

def check_changes(changes, changes_good):
    issues = []
    for [pin, change] in changes.items():
        if change != changes_good[pin]:
            issue = 'error:' + pin
            #issue += ' got:[' + ','.join([i for i in change]) + ']'
            #issue += ' expected:[' + ','.join([i for i in changes_good[pin]]) + ']'
            issue += ' missing:[' + ','.join([x for x in changes_good[pin] if x not in change]) + ']'
            issue += ' extra:[' + ','.join([x for x in change if x not in changes_good[pin]]) + ']'
            issues.append(issue)
    return issues

def record(pin_filename):
    pins = import_pins(pin_filename)
    changes = read_pin_states(pins)
    save_pin_changes(pin_filename, changes)

def test(pin_filename):
    pins = import_pins(pin_filename)
    changes_good = read_pin_changes(pin_filename)
    changes = read_pin_states(pins)
    return check_changes(changes, changes_good)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='GPIO short connector test')
    parser.add_argument('-pins', required=True, help='CSV file of pins to test')
    parser.add_argument('-r', '--record', action='store_true', help='Set to record a known good board')

    args = parser.parse_args()

    if args.record:
        record(args.pins)
    else:
        print('\n'.join(test(args.pins)))
