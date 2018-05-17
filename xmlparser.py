f = open(sys.argv[1])
i=0;

for line in f.readLines():
    print(i,line)
