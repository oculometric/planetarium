#include "fly_camera_node.h"

#include <string>

#if defined(_WIN32)
#include <windows.h>
#include <shobjidl.h> 

std::string runOpenDialog()
{
    std::string ret = "";
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        for (size_t i = 0; pszFilePath[i] != '\0'; i++)
                            ret.push_back(pszFilePath[i]);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return ret;
}
#endif

using namespace std;

#include "input.h"
#include "render_server.h"
#include "scene.h"
#include "application.h"

void PTFlyCameraNode::process(float delta_time)
{
    PTNode* parent = getScene()->findNode<PTNode>("parent");
    parent->getTransform()->rotate(delta_time * 30.0f, PTVector3f::forward(), parent->getTransform()->getPosition());
    PTNode* spot = getScene()->findNode<PTNode>("spot");
    spot->getTransform()->translate(PTVector3f{ 0, 0, sin(getApplication()->getTotalTime()) * 0.04f });

    PTInput* manager = PTInput::get();
    static PTVector2i mouse_pos = manager->getMousePosition();

    // set debug mode
    PTRenderServer::get()->debug_mode = manager->wasKeyPressed('F') || manager->getButtonState(PTGamepad::Button::CONTROL_SOUTH);
    // set screenshot wanted
    if (manager->wasKeyPressed('P') || manager->getButtonState(PTGamepad::Button::CONTROL_NORTH))
        PTRenderServer::get()->setWantsScreenshot();

    if (manager->wasKeyPressed('O') || manager->getButtonState(PTGamepad::Button::CONTROL_EAST))
    {
        string file_name = runOpenDialog();
        PTApplication::get()->openScene(file_name);
    }
        
    if (manager->wasMousePressed(PTInput::MouseButton::MOUSE_RIGHT))
        manager->setMouseVisible(false);
    if (manager->wasMouseReleased(PTInput::MouseButton::MOUSE_RIGHT))
        manager->setMouseVisible(true);

    PTVector2i mouse_delta = manager->getMousePosition() - mouse_pos;
    mouse_pos = manager->getMousePosition();

    float keyboard_x = 0;
    float keyboard_y = 0;
    float keyboard_z = 0;
    float keyboard_lx = 0;
    float keyboard_ly = 0;
    if (manager->isMouseDown(PTInput::MouseButton::MOUSE_RIGHT))
    {
        // keyboard movement vector
        keyboard_x = (float)(manager->isKeyDown('D')) - (float)(manager->isKeyDown('A'));
        keyboard_y = (float)(manager->isKeyDown('W')) - (float)(manager->isKeyDown('S'));
        keyboard_z = (float)(manager->isKeyDown('E')) - (float)(manager->isKeyDown('Q'));

        // keyboard look vector
        keyboard_lx = (float)mouse_delta.x / 10.0f;//(float)(manager->isKeyDown('J')) - (float)(manager->isKeyDown('L'));
        keyboard_ly = (float)mouse_delta.y / 10.0f;//(float)(manager->isKeyDown('I')) - (float)(manager->isKeyDown('K'));
    }
    
    // gamepad move vector
    PTVector2f move_axis = manager->getJoystickState(PTGamepad::Axis::LEFT_AXIS) + PTVector2f{ keyboard_x, -keyboard_y };
    
    // construct local-space movement vector from combined move vector
    PTVector3f local_movement = PTVector3f
    {
       move_axis.x,
       (-((float)manager->getButtonState(PTGamepad::Button::LEFT_MAJOR) - (float)manager->getButtonState(PTGamepad::Button::RIGHT_MAJOR))) + keyboard_z,
       move_axis.y,
    } * delta_time * 2.0f;

    // rotate local movement vector to world space
    PTVector3f world_movement = rotate(getTransform()->getLocalRotation(), local_movement);

    // apply movement, and update the debug
    getTransform()->translate(world_movement);
    debugSetSceneProperty("camera pos", to_string(getTransform()->getLocalPosition()));
    
    // total look vector (keyboard and gamepad combined)
    PTVector2f look_axis = manager->getJoystickState(PTGamepad::Axis::RIGHT_AXIS) + PTVector2f{ keyboard_lx, keyboard_ly };
    debugSetSceneProperty("look", to_string(-look_axis.x) + ',' + to_string(-look_axis.y));

    // apply rotation according to look vector, update debug
    getTransform()->rotate(look_axis.y * delta_time * 90.0f, getTransform()->getRight(), getTransform()->getPosition());
    getTransform()->rotate(look_axis.x * delta_time * 90.0f, PTVector3f::forward(), getTransform()->getPosition());
    debugSetSceneProperty("camera rot", to_string(getTransform()->getLocalRotation()));

    debugSetSceneProperty("camera for", to_string(getTransform()->getForward()));

    // modify fov
    horizontal_fov += ((float)manager->getButtonState(PTGamepad::Button::RIGHT_MINOR) - (float)manager->getButtonState(PTGamepad::Button::LEFT_MINOR)) * delta_time * 30.0f;
    horizontal_fov = max(min(horizontal_fov, 120.0f), 10.0f);
    debugSetSceneProperty("camera fov", to_string(horizontal_fov));
}

PTFlyCameraNode::PTFlyCameraNode(PTDeserialiser::ArgMap arguments) : PTCameraNode(arguments)
{ }