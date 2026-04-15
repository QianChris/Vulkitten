# 在项目根目录执行
mkdir build
cd build

# 生成VS2022工程 (x64)
cmake .. -G "Visual Studio 17 2022" -A x64

# 打开生成的解决方案
# start VulkittenEngine.sln