#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <algorithm>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window;
    VkInstance instance;

private:
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Triangle", nullptr, nullptr);
    }

    void createInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Triangular Sandboxing";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // figure out how many and which vk extensions GLFW requires.
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        // no val layers for now, but if we had gt 0 we would populate struct prop ppEnabledLayerNames.
        createInfo.enabledLayerCount = 0;

        if (!checkExtensions(std::vector<std::string>(glfwExtensions, glfwExtensions + glfwExtensionCount)))
        {
            throw std::runtime_error("failed to create triangly vkinstance due to unsupported extensions");
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create triangly vkinstance");
        }
    }

    /**
     * @brief Enumerates the supported vk extensions on stdout and checks them against the input required extension set.
     * @return true if all the required extensions are also found in the supported extensions set; false otherwise.
     */
    bool checkExtensions(const std::vector<std::string> requiredExtensions)
    {
        // check which vk extensions the driver supports against our requirements.
        uint32_t supportedExtensionCount = 0;
        // call in once to find out how many vk extensions the driver supports.
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
        // init vector of extension prop structs to the count of supported extensions we just learned.
        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        // call in again to populate the supported extensions via vector's C data interface.
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

        std::cout << "available extensions:\n";
        for (const auto &extension : supportedExtensions)
        {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        bool allRequiredExtensionsAreSupported = true;
        for (const auto &requiredExtension : requiredExtensions)
        {
            const auto it = std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [requiredExtension](VkExtensionProperties props)
                                         { return props.extensionName == requiredExtension; });
            allRequiredExtensionsAreSupported = allRequiredExtensionsAreSupported && it != supportedExtensions.end();
            if (allRequiredExtensionsAreSupported)
            {
                std::cout << "extension " << requiredExtension << " is supported." << '\n';
            }
            else
            {
                std::cerr << "extension " << requiredExtension << " is NOT supported." << '\n';
            }
        }
        return allRequiredExtensionsAreSupported;
    }

    void initVulkan()
    {
        createInstance();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}