#include "material.h"
#include "stb_image.h"

Image::Image(const std::string &file, const std::string &name) : name(name) {
	stbi_set_flip_vertically_on_load(1);
	uint8_t *data = stbi_load(file.c_str(), &width, &height, &channels, 4);
	channels = 4;
	if (!data) {
		throw std::runtime_error("Failed to load " + file);
	}
	img = std::vector<uint8_t>(data, data + width * height * channels);
	stbi_image_free(data);
}
