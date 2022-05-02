#ifndef GOCONTAINER_H
#define GOCONTAINER_H

//#define SLICE_CAPACITY_LIMIT
#define SLICE_CAPACITY_APPEND_LIMIT 1024*1024 // 1M
#define SLICE_CAPACITY_APPEND_LEN 1024 // 1K

#include <regex>

namespace go{
    template<typename T>
    class slice{
    public:
        std::unique_ptr<T[]> head;
        size_t size;
        size_t capacity;

        T& operator[](size_t index){
            if(index>=size || index<0)
                throw std::out_of_range("index out of range");
            return head[index];
        }

        bool empty() { return this->head == nullptr; }
        void append(const T& t){
            // capacity expansion
            if(size == capacity){
                size_t new_capacity = capacity;
                new_capacity += capacity<SLICE_CAPACITY_APPEND_LIMIT?
                        capacity/2 : SLICE_CAPACITY_APPEND_LEN;

                std::unique_ptr<T[]> new_head (new T[new_capacity]());
                memcpy(new_head.get(), head.get(), size*sizeof(T));
                head = std::move(new_head);
            }
            head.get()[size] = t;
            size++;
        }

        slice() : size(0), capacity(0), head(nullptr){}
        slice(size_t size) : size(size), capacity(size), head(new T[size]()){}
        slice(size_t size, T t) : slice(size){
            for(int i=0;i<size;i++)
                head[i] = t;
        }
        // copy constructor
        slice(slice<T>& t) : size(t.size), capacity(t.capacity), head(new T[t.size]()){
            for(int i=0;i<size;i++)
                head[i] = t[i];
        }
        // move constructor
        slice(slice<T>&& t) : size(t.size), capacity(t.capacity){
            head = std::move(t.head);
            t.head = nullptr;
        }
        ~slice(){}
    };

    /*
     * golang like slice class factory
     */
    template<typename T>
    static slice<T>&& make_slice(){
        return slice<T>();
    }
    template<typename T>
    static slice<T> make_slice(T array[], int len){
        slice<T> slice_ans(len);
        for(int i=0;i<len;i++){
            slice_ans[i] = array[i];
        }
        return slice_ans;
    }
    template<typename T>
    static slice<T> make_slice(T array[], int len, int capacity){
        slice<T> slice_ans(len);
        slice_ans.capacity = capacity;
        for(int i=0;i<len;i++){
            slice_ans[i] = array[i];
        }
        return slice_ans;
    }
}

#endif GOCONTAINER_H
