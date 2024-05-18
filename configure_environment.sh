# this is all designed to run under Ubuntu. i cannot be fucked to make it platform-agnostic right now

sudo apt install vulkan-tools
sudo apt install libvulkan-dev
sudo apt install vulkan-validationlayers-dev spirv-tools

sudo apt install libglfw3-dev
sudo apt install libglm-dev

sudo apt install libxxf86vm-dev libxi-dev

wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.3.283-jammy.list https://packages.lunarg.com/vulkan/1.3.283/lunarg-vulkan-1.3.283-jammy.list
sudo apt update
sudo apt install vulkan-sdk