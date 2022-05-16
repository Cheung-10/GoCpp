#ifndef GOSLICE_H
#define GOSLICE_H

#include <regex>

//#define SLICE_CAPACITY_LIMIT
#define SLICE_CAPACITY_APPEND_LIMIT 1024*1024 // 1M
#define SLICE_CAPACITY_APPEND_LEN 1024 // 1K

namespace go{
    template<typename T>
    class slice{
    private:
        T **head = new T*;
        std::mutex *head_mtx = new std::mutex;
        size_t begin = 0;

        slice(const size_t size) : size(size), capacity(size){
            *head = new T[capacity];
        }
        void capacityExpansion(){
            std::lock_guard<std::mutex> mtx(*head_mtx);
            size_t new_capacity = capacity;
            new_capacity += capacity<SLICE_CAPACITY_APPEND_LIMIT?
                            capacity/2==0? 1:capacity/2 : SLICE_CAPACITY_APPEND_LEN;

            T* new_head = new T[new_capacity];
            for(int i=0;i<size;i++){
                new_head[i] = std::move((*head)[begin+i]);
            }

            capacity = new_capacity;

            if(original){
                std::swap(*head, new_head);
                delete[] new_head;
            }
            else{
                // set new head
                head = new T*;
                *head = new_head;
                // set a new mtx
                head_mtx = new std::mutex;

                original = true;
                begin=0;
            }
        }
    public:
        bool original = true;
        size_t size = 0;
        size_t capacity = 0;

        T& operator[](const size_t index){
            if(index>=size || index<0)
                throw std::out_of_range("index out of range");
            return (*head)[begin+index];
        }
        slice& operator=(slice& t){
            if(this->original){
                if(head && *head) delete[] *head;
                if(head) delete head;
                if(head_mtx) delete head_mtx;
            }
            this->original = false;
            this->head = t.head;
            this->head_mtx = t.head_mtx;
            this->begin = t.begin;
            this->size = t.size;
            this->capacity = t.capacity;
        }
        slice& operator=(slice&& t){
            if(this->original){
                if(head && *head) delete[] *head;
                if(head) delete head;
                if(head_mtx) delete head_mtx;
            }
            this->original = t.original;
            this->head = t.head;
            this->head_mtx = t.head_mtx;
            this->begin = t.begin;
            this->size = t.size;
            this->capacity = t.capacity;

            t.original = false;
            t.head = nullptr;
            t.head_mtx = nullptr;
        }

        bool empty() { return size == 0; }
        void append(T&& t){
            //capacity expansion
            if(size==capacity)
                capacityExpansion();

            (*head)[begin+size] = std::forward<T>(t);
            size++;
        }

        // TODO delete
        void printInfo(){
            std::cout<<"size\t\t"<<size<<std::endl;
            std::cout<<"capacity\t"<<capacity<<std::endl;
            std::cout<<"original\t"<<original<<std::endl;
            std::cout<<"begin\t\t"<<begin<<std::endl;
            std::cout<<"head\t\t"<<head<<std::endl;
            std::cout<<"*head\t\t"<<(head?*head:0)<<std::endl;
            std::cout<<"head_mtx\t"<<head_mtx<<std::endl;
        }

        slice(){ *head = nullptr; }
        slice(const size_t size, T t) : slice(size){
            for(int i=0;i<size;i++)
                (*head)[i] = t;
        }
        slice(const std::initializer_list<T>& elements) : slice(elements.size()){
            int i=0;
            for(auto it=elements.begin(); it!=elements.end(); it++){
                (*head)[i++] = *it;
            }
        }

        //copy constructor
        slice(slice<T>& t) : size(t.size), capacity(t.capacity), original(t.original){
            if(original){
                head_mtx = new std::mutex;
                *head = new T[capacity];
                for(int i=0;i<size;i++)
                    (*head)[i] = t[i];
            }
            else{
                head_mtx = t.head_mtx;
                head = t.head;
                begin = t.begin;
            }
        }
        // move constructor
        slice(slice<T>&& t) : size(t.size), capacity(t.capacity), original(t.original),
                                    head(t.head), head_mtx(t.head_mtx), begin(t.begin){
            t.size = 0;
            t.capacity = 0;
            t.original = false;
            t.head_mtx = nullptr;
            t.head = nullptr;
        }

        template<typename K>
        friend slice<K> make_slice(slice<K>& t, size_t begin, size_t end);

        ~slice(){
            if(original){
                if(head && *head) delete[] *head;
                if(head) delete head;
                if(head_mtx) delete head_mtx;
            }
        }
    };

    /*
     * golang like slice class factory
     */
    template<typename T>
    static slice<T> make_slice(){
        return slice<T>();
    }

    template<typename T>
    static slice<T> make_slice(slice<T>& t, size_t start, size_t end) {
        if(start<0 || end>t.size)
            return slice<T>();

        slice<T> slice_ans;

        slice_ans.size = end-start;
        slice_ans.capacity = t.capacity-start;

        slice_ans.begin = start+t.begin;
        slice_ans.original = false;

        *(slice_ans.head) = *(t.head);
        slice_ans.head_mtx = t.head_mtx;

        return slice_ans;
    }
}

#endif GOSLICE_H
