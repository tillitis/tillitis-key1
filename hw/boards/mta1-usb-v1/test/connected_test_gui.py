#!/usr/bin/python3
import tkinter as tk
from tkinter import filedialog as fd
from tkinter.messagebox import askyesno
import connected_test
import datetime

def handle_choose_file():
    filetypes = (
        ('CSV files', '*.csv'),
    )

    filename = fd.askopenfilename(
        title='Open a GPIO definition file',
        initialdir='.',
        filetypes=filetypes)

    print(filename)
    filename_entry.delete(0, 'end')
    filename_entry.insert(0, filename)

def handle_record():
    filename = filename_entry.get()
    [filename,x] = filename.split('.') 

    answer = askyesno(title='Record GPIO states',
                    message='Record GPIO states? This will delete any previous state recordings')

    if answer:
        results_text.delete("1.0", "end")  # if you want to remove the old data
        results_text.insert('end',datetime.datetime.now())
        results_text.insert('end',' recording gpio connections\n')

        connected_test.record(filename)

        changes_good = connected_test.read_pin_changes(filename)
        for [pin, change] in changes_good.items():
            results_text.insert('end','{:} [{:}]\n'.format(pin,','.join(change)))

def handle_test():
    filename = filename_entry.get()
    [filename,x] = filename.split('.') 

    results_text.delete("1.0", "end")  # if you want to remove the old data
    results_text.insert('end',datetime.datetime.now())
    results_text.insert('end',' testing gpio connections\n')
    results = connected_test.test(filename)

    if results == []:
        results = ['OK']

    results_text.insert('end','\n'.join(results))

window = tk.Tk()
window.title('Pin change tester')
#window.geometry('1600x1200')
frame = tk.Frame()

# Results window will show the test result
results_text = tk.Text(master=frame, height=30,width=180)
results_text.pack()

choosefile_button = tk.Button(master=frame, text='Choose file', command=handle_choose_file)
choosefile_button.pack()

label = tk.Label(master=frame, text='filename:')
label.pack();
filename_entry = tk.Entry(master=frame, width=80)
filename_entry.pack();

record_button = tk.Button(master=frame, text='record', command=handle_record)
record_button.pack()

test_button = tk.Button(master=frame, text='test', command=handle_test)
test_button.pack()

frame.pack()
window.mainloop()
