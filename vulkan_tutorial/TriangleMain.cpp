#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include <optional>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{

    std::cerr << "The validation layer says: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

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
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    /**
     * A queue in Vulkan is a literal queue of commands; we send command buffers to them, and each queue in each queue family is ordered. The queue family we're interested in is just graphics, so the idea is to have command queues per GPU hardware capability e.g. graphics, compute, codec ops etc. See https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueueFlagBits.html for the full list of queue family cap bits.
     */
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        bool isComplete()
        {
            return graphicsFamily.has_value();
        }
    };

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
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // debug message struct fella has to live outside the val condition because it will be referenced by creatinfo and that will be used by the call to vkCreateInstance()
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (!checkExtensions(std::vector<std::string>(extensions.data(), extensions.data() + extensions.size())))
        {
            throw std::runtime_error("failed to create triangly vkinstance due to unsupported extensions");
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create triangly vkinstance");
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        // Assign index to queue families that could be found
        // This business of double calls to every query API endpoint because we need to get the result count and perform population separately is an extremely acceptable API design. Good one, Khronos smdh.
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // todo: the index we actually get outta this seems arbitrary, unless the queuefamilies we get above are ordered for some reason, and even if they are why not get the index from the actual populated struct data rather than counting manually ourselves? I don't like the smell.
        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
                break;
            }

            i++;
        }

        return indices;
    }

    /**
     * @return the set of extension names we need to support GLFW and optionally debugging in debug builds.
     */
    std::vector<const char *> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
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
        setupDebugMessenger();
        createSurface();
        // Physical device represents the actual hardware capabilities available for Vulkan
        pickPhysicalDevice();
        // Logical device is how we interace with the physical device; there can be many logical devices interfacing with one physical device and they maintain independent states
        createLogicalDevice();
    }

    void createSurface() 
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{}; // todo: revisit when we finally start doing anything

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }
        vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        /* first come algo
        for (const auto &device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        */

        // Use an ordered map to automatically sort candidates by increasing score
        std::multimap<int, VkPhysicalDevice> candidates;

        for (const auto &device : devices)
        {
            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }

        // Check if the best candidate is suitable at all
        if (candidates.rbegin()->first > 0)
        {
            physicalDevice = candidates.rbegin()->second;
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
            std::cout << "Selecting GPU device " << deviceProperties.deviceName << '\n';
        }
        else
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               deviceFeatures.geometryShader;
    }

    int rateDeviceSuitability(VkPhysicalDevice device)
    {
        int score = 0;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        std::cout << "Considering gpu device " << deviceProperties.deviceName << '\n';

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        // Application can't function without geometry shaders or required Q family(ies)
        QueueFamilyIndices indices = findQueueFamilies(device);
        if (!deviceFeatures.geometryShader || !indices.isComplete())
        {
            return 0;
        }

        return score;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
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
        vkDestroyDevice(logicalDevice, nullptr);
        if (enableValidationLayers)
        {
            destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(instance, surface, nullptr);
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