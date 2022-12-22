#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped CSV BOM
#

"""
    @package
    Output: CSV (comma-separated)
    Grouped By: Value, Extended Value, Footprint
    Sorted By: Ref
    Fields: Ref, Qnty, Value, Footprint, Description, Manufacturer, Manufacturer Part Number

    Command line:
    python "pathToFile/bom_csv_grouped_by_extended_value.py" "%I" "%O.csv"
"""

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import kicad_utils
import csv
import sys

# A helper function to convert a UTF8/Unicode/locale string read in netlist
# for python2 or python3
def fromNetlistText( aText ):
    if sys.platform.startswith('win32'):
        try:
            return aText.encode('utf-8').decode('cp1252')
        except UnicodeDecodeError:
            return aText
    else:
        return aText

# Group components if their value, extended value, footprint, and reference designator type (?) are the same
import string
def equate_value_extended_value_footprint(self, other):
    """ Equivalency operator, remember this can be easily overloaded
        2 components are equivalent ( i.e. can be grouped
        if they have same value and same footprint

        Override the component equivalence operator must be done before
        loading the netlist, otherwise all components will have the original
        equivalency operator.

        You have to define a comparison module (for instance named myEqu)
        and add the line;
            kicad_netlist_reader.comp.__eq__ = myEqu
        in your bom generator script before calling the netliste reader by something like:
            net = kicad_netlist_reader.netlist(sys.argv[1])
    """
    result = False
    if self.getValue() == other.getValue():
        if self.getField("Extended Value") == other.getField("Extended Value"):
            if self.getFootprint() == other.getFootprint():
                if self.getRef().rstrip(string.digits) == other.getRef().rstrip(string.digits):
                    result = True
    return result

kicad_netlist_reader.comp.__eq__ = equate_value_extended_value_footprint

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
try:
    f = kicad_utils.open_file_write(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, sys.stderr)
    f = sys.stdout

# Create a new csv writer object to use as the output formatter
out = csv.writer(f, delimiter=',', lineterminator='\n', quotechar='\"', quoting=csv.QUOTE_ALL)

# Output a set of rows for a header providing general information
out.writerow(['Source:', net.getSource()])
out.writerow(['Date:', net.getDate()])
out.writerow(['Tool:', net.getTool()])
out.writerow( ['Generator:', sys.argv[0]] )
out.writerow(['Component Count:', len(net.components)])
out.writerow(['Ref', 'Qnty', 'Value', 'Footprint', 'Description', 'Manufacturer', 'Manufacturer Part Number', 'Supplier', 'Supplier Part Number'])


# Get all of the components in groups of matching parts + values
# (see ky_generic_netlist_reader.py)
grouped = net.groupComponents()

# Output all of the component information
for group in grouped:
    refs = ','.join([fromNetlistText( component.getRef() ) for component in group])
    c = group[-1]

    combinedvalue = c.getValue()
    if c.getField("Extended Value") != '':
        combinedvalue += ',' + c.getField("Extended Value")

    # Fill in the component groups common data
    out.writerow([
        refs,
        len(group),
        combinedvalue,
        fromNetlistText( c.getFootprint() ),
        fromNetlistText( c.getDescription() ),
        fromNetlistText( c.getField("Manufacturer") ),
        fromNetlistText( c.getField("Manufacturer Part Number") ),
        fromNetlistText( c.getField("Supplier") ),
        fromNetlistText( c.getField("Supplier Part Number") )
    ])


