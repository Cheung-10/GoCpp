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
        // head指向数组指针，后者指向保存数据的数组
        // head_mtx为修改head时需要的互斥量
        // begin为当前切片的实际开始下标
        T **head = new T*;
        std::mutex *head_mtx = new std::mutex;
        size_t begin = 0;

        slice(const size_t size) : size(size), capacity(size){
            *head = new T[capacity];
        }

        void capacityExpansion(){
            std::lock_guard<std::mutex> mtx(*head_mtx);
            // 新容量为之前容量的1.5倍，至少增加1,大于限制时只增加固定值
            // 不设置容量上限（可能引发容量溢出错误）
            size_t new_capacity = capacity;
            new_capacity += capacity<SLICE_CAPACITY_APPEND_LIMIT?
                            capacity/2==0? 1:capacity/2 : SLICE_CAPACITY_APPEND_LEN;

            T* new_head = new T[new_capacity];
            capacity = new_capacity;
            for(int i=0;i<size;i++){
                // 移动原有的值到新的数组
                new_head[i] = std::move((*head)[begin+i]);
            }

            if(original){
                std::swap(*head, new_head);
                delete[] new_head;
            }
            else{
                // 令head指向新的一级指针
                head = new T*;
                *head = new_head;
                // 设置新的head_mtx
                head_mtx = new std::mutex;

                original = true;
                begin=0;
            }
        }
    public:
        // original 说明是否是原始切片，原始切片需要负责堆资源的释放
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

        void append(T& t){
            if(size==capacity)
                capacityExpansion();
            // t为左值，应构造新的匿名实例并保存
            (*head)[begin+size] = T(t);
            size++;
        }
        void append(T&& t){
            if(size==capacity)
                capacityExpansion();
            // 转发t的引用，一般是调用右值赋值运算
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
        slice(std::initializer_list<T>&& elements) : slice(elements.size()){
            int i=0;
            for(auto it=elements.begin(); it!=elements.end(); it++){
                // 如果init_list中保存的是类变量，则其类型为 const type
                // 此时需要取消const类型并绑定到右值引用进行移动赋值,否则会有指向临时变量的风险
                (*head)[i++] = std::move(const_cast<T&>(*it));
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

    // 对已有切片再次切片
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
