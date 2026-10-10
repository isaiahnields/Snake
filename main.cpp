#define GLFW_EXPOSE_NATIVE_WIN32

#include <stdio.h>
#include "bgfx/bgfx.h"
#include "GLFW/glfw3.h"
#include <GLFW/glfw3native.h>


#define WNDW_WIDTH 1600
#define WNDW_HEIGHT 900


int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(WNDW_WIDTH, WNDW_HEIGHT, "Hello, bgfx!", NULL, NULL);

    bgfx::Init init;
    init.type = bgfx::RendererType::Count;

    #if defined(_WIN32)
        init.swapChain.nwh = glfwGetWin32Window(window);
    #elif defined(__linux__)
        init.swapChain.ndt = glfwGetX11Display();
        init.swapChain.nwh = (void*)(uintptr_t)glfwGetX11Window(window);
    #endif

    init.swapChain.width = WNDW_WIDTH;
    init.swapChain.height = WNDW_HEIGHT;
;
    init.reset = BGFX_RESET_VSYNC;

    // Call init directly with your configured structure
    bgfx::init(init);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, WNDW_WIDTH, WNDW_HEIGHT);

    // 4. Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Touch view 0 to clear it even with no draw calls
        bgfx::touch(0);
        bgfx::frame();
    }

    // 5. Cleanup
    bgfx::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}