# Monte Carlo Pi Estimation
A simple visualization of estimating π using the Monte Carlo method.

Uses a unit circle (radius 1) centered at origin inside a 2×2 square (from -1,-1 to 1,1). Throws random points at the square and counts how many land inside the circle. Since the circle has area π and the square has area 4, the ratio (points inside / total points) approximates π/4. Multiply by 4 to get π!

## Building
```bash
git clone --recursive https://github.com/imisumi/monte-carlo-pi-estimation.git
cd monte-carlo-pi-estimation
mkdir build && cd build
cmake ..
cmake --build . --config Release
