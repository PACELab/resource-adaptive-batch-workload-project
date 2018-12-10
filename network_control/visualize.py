import matplotlib.pyplot as plt


x_axis = []
y_mean = []
y_upstdev = []
y_downstdev = []
y_curbw = []
y_upper = []
y_lower = []

# TODO: change logic to check lines with a marker only
# For now, do not print any other lines in network-control.cpp
# which might break this script

# Input the output file from network-control.cpp
with open('results.txt') as fp:
    time = 0
    for line in fp:
        # Hack to skip the last line which states violations and phase changes
        if 'Violations' in line:
            continue

        time += 1
        mean, upstdev, downstdev, curbw, upper, lower = list(map(float, line.split(" ")))

        x_axis.append(time)
        y_mean.append(mean)
        y_upstdev.append(upstdev)
        y_downstdev.append(downstdev)
        y_curbw.append(curbw)
        y_upper.append(upper)
        y_lower.append(lower)

plt.plot(x_axis, y_mean, color='black')
plt.plot(x_axis, y_upstdev, color='black')
plt.plot(x_axis, y_downstdev, color='black')
plt.plot(x_axis, y_lower, color='red')
plt.plot(x_axis, y_curbw, color='green')
plt.plot(x_axis, y_upper, color='red')
plt.show()
