import numpy as np

def snap_point_near_x(x_arr, y_arr, x):
    x_arr = np.asarray(x_arr)
    y_arr = np.asarray(y_arr)
    mask = np.isfinite(x_arr) & np.isfinite(y_arr)
    x_arr = x_arr[mask]; y_arr = y_arr[mask]
    if x_arr.size == 0:
        return None
    i = int(np.argmin(np.abs(x_arr - x)))
    return float(x_arr[i]), float(y_arr[i])

def nearest_line_snap(lines, event):
    if event.xdata is None or event.x is None or event.y is None:
        return None
    x = float(event.xdata)
    mx, my = float(event.x), float(event.y)

    best = None
    for line in lines:
        if line is None or (not line.get_visible()):
            continue
        xd = line.get_xdata(orig=False)
        yd = line.get_ydata(orig=False)
        snapped = snap_point_near_x(xd, yd, x)
        if snapped is None:
            continue
        xs, ys = snapped
        ax = line.axes
        px, py = ax.transData.transform((xs, ys))
        dist = ((px-mx)**2 + (py-my)**2) ** 0.5
        if best is None or dist < best[0]:
            best = (dist, line, xs, ys)
    return None if best is None else (best[1], best[2], best[3])
