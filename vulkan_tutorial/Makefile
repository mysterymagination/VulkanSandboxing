CFLAGS = -std=c++20 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: VulkanTest.cpp
	g++ $(CFLAGS) -o Build/VulkanTest VulkanTest.cpp $(LDFLAGS)

VulkanTriangle: TriangleMain.cpp
	g++ $(CFLAGS) -o Build/VulkanTriangle TriangleMain.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	./Build/VulkanTest

triangle: VulkanTriangle
	./Build/VulkanTriangle

clean:
	rm -f Build/*