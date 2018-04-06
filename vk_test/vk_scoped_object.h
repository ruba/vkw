#pragma once

#include <vector>
#include <functional>

namespace vkw
{
    template <typename T>
    struct VkScopedObject
    {
    public:
        VkScopedObject()
        : object_(nullptr)
        , deleter_(nullptr)
        {
        }
        
        VkScopedObject(std::nullptr_t)
        : object_(nullptr)
        , deleter_(nullptr)
        {
        }
        
        template <typename F> VkScopedObject(T object, F deleter)
        : object_(object)
        , deleter_(deleter)
        {}
        
        VkScopedObject(VkScopedObject&& rhs)
        : object_(rhs.object_)
        , deleter_(rhs.deleter_)
        {
            rhs.object_ = nullptr;
            rhs.deleter_ = nullptr;
        }
        
        VkScopedObject(VkScopedObject const&) = delete;
        
        VkScopedObject& operator=(VkScopedObject&& rhs)
        {
            std::swap(object_, rhs.object_);
            std::swap(deleter_, rhs.deleter_);
            return *this;
        }
        
        VkScopedObject& operator=(VkScopedObject const&) = delete;
        
        ~VkScopedObject()
        {
            if (object_ && deleter_)
            {
                deleter_(object_);
            }
        }
        
        operator T() { return object_; }
        
    private:
        T object_;
        std::function<void(T)> deleter_;
    };
    
    template <typename T>
    struct VkScopedArray
    {
    public:
        VkScopedArray()
        : deleter_(nullptr)
        {
        }
        
        VkScopedArray(std::nullptr_t)
        : deleter_(nullptr)
        {
        }
        
        template <typename F> VkScopedArray(T* objects, std::uint32_t size, F deleter)
        : objects_(objects, objects + size)
        , deleter_(deleter)
        {}
        
        VkScopedArray(VkScopedArray&& rhs)
        : objects_(std::move(rhs.objects_))
        , deleter_(std::move(rhs.deleter_))
        {
            rhs.objects_.clear();
            rhs.deleter_ = nullptr;
        }
        
        VkScopedArray(VkScopedArray const&) = delete;
        
        VkScopedArray& operator=(VkScopedArray&& rhs)
        {
            std::swap(objects_, rhs.objects_);
            std::swap(deleter_, rhs.deleter_);
            return *this;
        }
        
        VkScopedArray& operator=(VkScopedArray const&) = delete;
        
        ~VkScopedArray()
        {
            if (!objects_.empty() && deleter_)
            {
                deleter_(objects_.data(), (std::uint32_t)objects_.size());
            }
        }
        
        operator std::vector<T> const& () const { return objects_; }
        T operator [](int i) const { return objects_[i]; }
        
        auto size() const { return objects_.size(); }
        auto data() const { return objects_.data(); }
        
    private:
        std::vector<T> objects_;
        std::function<void(T*, std::uint32_t)> deleter_;
    };
}

