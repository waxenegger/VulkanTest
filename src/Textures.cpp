#include <src/includes/models.h>

int Texture::getId() {
    return this->id;
}

std::string Texture::getType() {
    return this->type;
}

bool Texture::isValid() {
    return this->valid;
}

VkFormat Texture::getImageFormat() {
    return this->imageFormat;
}

VkImageView & Texture::getTextureImageView() {
    return this->textureImageView;
}

void Texture::setId(const int & id) {
    this->id = id;
}

void Texture::setType(const std::string & type) {
    this->type = type;
}

void Texture::setPath(const std::filesystem::path & path) {
    this->path = path;
}

void Texture::setTextureImage(VkImage & image) {
    this->textureImage = image;
}

void Texture::setTextureImageMemory(VkDeviceMemory & imageMemory) {
    this->textureImageMemory = imageMemory;
}

void Texture::setTextureImageView(VkImageView & imageView) {
    this->textureImageView = imageView;
}

void Texture::load() {
    if (!this->loaded) {
        this->textureSurface = IMG_Load(this->path.c_str());
        if (this->textureSurface != nullptr) {
            if (!this->readImageFormat()) {
                std::cout << "Unsupported Texture Format: " << this->path << std::endl;
            } else if (this->getSize() != 0) {
                this->valid = true;
            }
        } else std::cout << "Failed to load texture: " << this->path << std::endl;
        this->loaded = true;
    }
}

void Texture::cleanUpTexture(const VkDevice & device) {
    if (device == nullptr) return;
    
    if (this->textureImage != nullptr) {
        vkDestroyImage(device, this->textureImage, nullptr);        
        this->textureImage = nullptr;
    }

    if (this->textureImageMemory != nullptr) {
        vkFreeMemory(device, this->textureImageMemory, nullptr);
        this->textureImageMemory = nullptr;
    }

    if (this->textureImageView != nullptr) {
        vkDestroyImageView(device, this->textureImageView, nullptr);
        this->textureImageView = nullptr;
    }
}

uint32_t Texture::getWidth() {
    return this->textureSurface == nullptr ? 0 : this->textureSurface->w; 
}

uint32_t Texture::getHeight() {
    return this->textureSurface == nullptr ? 0 : this->textureSurface->h;     
}

VkDeviceSize Texture::getSize() {
    int channels = this->textureSurface == nullptr ? 0 : textureSurface->format->BytesPerPixel;
    return this->getWidth() * this->getHeight() * channels;
}

void * Texture::getPixels() {
 return this->textureSurface == nullptr ? nullptr : this->textureSurface->pixels;
}

void Texture::freeSurface() {
    if (this->textureSurface != nullptr) {
        SDL_FreeSurface(this->textureSurface);
        this->textureSurface = nullptr;
    }    
}

Texture::Texture(bool empty,  VkExtent2D extent) {
    if (empty) {
        this->textureSurface = SDL_CreateRGBSurface(0,extent.width, extent.height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

        this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        
        this->loaded = true;
        this->valid = this->textureSurface != nullptr;
    }
}

Texture::Texture(SDL_Surface * surface) {
    if (surface != nullptr) {
        this->textureSurface = surface;
        this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        
        this->loaded = true;
        this->valid = true;
    }
}

Texture::~Texture() {
    this->freeSurface();
}

bool Texture::readImageFormat() {
    if (this->textureSurface == nullptr) return false;

    const int nOfColors = this->textureSurface->format->BytesPerPixel;

    if (nOfColors == 4) {
        if (this->textureSurface->format->Rmask == 0x000000ff) this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        else this->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    } else if (nOfColors == 3) {
        Uint32 aMask = 0x000000ff;
        if (this->textureSurface->format->Rmask == 0x000000ff) this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        else {
            this->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
            aMask = 0xff000000;
        }
                
        // convert to 32 bit
        SDL_Surface * tmpSurface = SDL_CreateRGBSurface(
            0, this->textureSurface->w, this->textureSurface->h, 32, 
            this->textureSurface->format->Rmask, this->textureSurface->format->Gmask, this->textureSurface->format->Bmask, aMask);

        // attempt twice with different pixel format
        if (tmpSurface == nullptr) {
            std::cerr << "Conversion Failed. Try something else for: " << this->path << std::endl;

            tmpSurface = SDL_CreateRGBSurface(
                0, this->textureSurface->w, this->textureSurface->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);                

            if (tmpSurface == nullptr) {
                tmpSurface = SDL_CreateRGBSurface(
                    0, this->textureSurface->w, this->textureSurface->h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);                
            }
        }
        
        // either conversion has worked or not
        if (tmpSurface != nullptr) {            
            SDL_SetSurfaceAlphaMod(tmpSurface, 0);
            if (SDL_BlitSurface(this->textureSurface, nullptr, tmpSurface , nullptr) == 0) {
                SDL_FreeSurface(this->textureSurface);
                this->textureSurface = tmpSurface;
            } else {
                std::cerr << "SDL_BlitSurface Failed (on conversion): " << SDL_GetError() << std::endl;
                return false;
            }
        } else {
            std::cerr << "SDL_CreateRGBSurface Failed (on conversion): " << SDL_GetError() << std::endl;
            return false;
        }
    } else return false;

    return true;
}

