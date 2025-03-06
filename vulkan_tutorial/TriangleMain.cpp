#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <algorithm>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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
        // I'm having a disagreement with the tutorial on how best to handle the C/C++ interop since Vulkan is a C API but our program is C++... well anyway, it's easier to build up wrappers around pure data than to decompose wrappers into the appropriate data, so let's go with cstring constant and then build a wrapper of wrappers from that why not. Technically we're creating an opportunity for a disconnect between what layers we check for and what we actually request, but I don't wanna use strcmp and friends in the analysis. I don't wanna!
        std::vector<std::string> validationLayerStrings;
        for (auto cstring : validationLayers)
        {
            validationLayerStrings.emplace_back(std::string(cstring));
        }

        if (enableValidationLayers && !checkValidationLayerSupport(validationLayerStrings))
        {
            throw std::runtime_error("some desired validation layers are not available!");
        }

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
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

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
     * @brief checks if the input validation layers are supported by the driver.
     * @input reference to a string vector of desired validation layers.
     * @return true if all desired layers are supported; false otherwise.
     */
    bool checkValidationLayerSupport(const std::vector<std::string> &desiredValidationLayers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        std::cout << "available val layers:\n";
        for (const auto &layer : availableLayers)
        {
            std::cout << '\t' << layer.layerName << '\n';
        }

        bool allDesiredLayersAreSupported = true;
        for (const auto &desiredLayer : desiredValidationLayers)
        {
            const auto it = std::find_if(availableLayers.begin(), availableLayers.end(), [desiredLayer](VkLayerProperties layerProps)
                                         { return layerProps.layerName == desiredLayer; });
            allDesiredLayersAreSupported = allDesiredLayersAreSupported && it != availableLayers.end();
            if (allDesiredLayersAreSupported)
            {
                std::cout << "layer " << desiredLayer << " is supported." << '\n';
            }
            else
            {
                std::cerr << "layer " << desiredLayer << " is NOT supported." << '\n';
            }
        }
        return allDesiredLayersAreSupported;
    }

    /**
     * @brief Enumerates the supported vk extensions on stdout and checks them against the input required extension set.
     * @param reference to a vector of strings identifying the required extension names.
     * @return true if all the required extensions are also found in the supported extensions set; false otherwise.
     */
    bool checkExtensions(const std::vector<std::string> &requiredExtensions)
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