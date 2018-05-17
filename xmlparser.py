import xml.etree.ElementTree as ET
import sys

tree = ET.parse(sys.argv[1])


for child in tree.getroot():
    prop = child.find('name')
    if(prop.text == 'yarn.scheduler.maximum-allocation-mb'):
        prop.value = int(sys.argv[2])

tree.write(sys.argv[3])
