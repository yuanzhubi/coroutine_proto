#ifndef CORT_PROTO_H_
#define CORT_PROTO_H_

#define CO_JOIN2(x,y) x##y 
#define CO_JOIN(x,y) CO_JOIN2(x,y)

#define CO_STATE_MAX_COUNT ((count_type)((1u<<(sizeof(count_type)*8)) - 1u)) // you can increase the number if your compiler affordable

#define CO_STATE_EVAL_COUNTER(counter) (sizeof(*counter((struct_int<CO_STATE_MAX_COUNT>*)0)) \
          - sizeof(*counter((void*)0)))
          
/*We can change the result of CO_STATE_EVAL_COUNTER if we use CO_STATE_INCREASE_COUNTER or CO_STATE_SET_COUNTER*/

#define CO_STATE_INCREASE_COUNTER(counter, delta)  static char (*counter(struct_int<CO_STATE_EVAL_COUNTER(counter) + 1>*))[CO_STATE_EVAL_COUNTER(counter) + sizeof(*counter((void*)0)) + (delta)] 

#define CO_STATE_SET_COUNTER(counter, value)  static char (*counter(struct_int<CO_STATE_EVAL_COUNTER(counter) + 1>*))[value + sizeof(*counter((void*)0))]

struct cort_proto{ 
    typedef cort_proto* (*run_type)(cort_proto*);
    typedef cort_proto base_type;

    void set_run_function(run_type arg){
        data0.run_function = arg;
    }
    
    void set_parent(cort_proto *arg){
        cort_parent = arg;
    }
    
    void set_wait_count(size_t wait_count){
        this->data1.wait_count = wait_count;
    }
    
    void incr_wait_count(size_t wait_count){
        this->data1.wait_count += wait_count;
    }
    
    //Only useful for some rare case. Its overload form is called ususally.
    //It is behave like a virtual function. So if you want to call cort_proto::start, 
    //you MUST call "init" function in subclass first to initialize the "virtual table pointer".
    cort_proto* start() {
        return (*(this->data0.run_function))(this);
    }
    
    //Some times outer code want this coroutine to wait some more coroutines that it does not know.
    //Await can be only called after "this" coroutine is awaiting at least one coroutine now or CO_AWAIT_ANY.
    //It must is guranteed that the "this" coroutine is alive when subcoroutine is finished.
    template<typename T>
    cort_proto* await(T* sub_cort){
        cort_proto* __the_sub_cort = sub_cort->start();
        if(__the_sub_cort != 0){
            __the_sub_cort->set_parent(this); 
            this->incr_wait_count(1); 
            return this; 
        }
    }
    
    void resume() {
        if((*(this->data0.run_function))(this) == 0 && cort_parent != 0 && (--(cort_parent->data1.wait_count)) == 0){
            cort_parent->resume();
        }
    }
    
    void clear(){
        cort_parent = 0;
    }

private:
    union{
        run_type run_function;
        void* result_ptr;                   //Useful to save coroutine result
        size_t result_int;                  //Useful to save coroutine result
    }data0;
    union{
        size_t wait_count;
        void* result_ptr;                   //Useful to save coroutine result
        size_t result_int;                  //Useful to save coroutine result
    }data1;
    cort_proto* cort_parent;
    
protected:
    cort_proto():cort_parent(0){}   
    
    ~cort_proto(){}                 //only used as weak reference so public virtual destructor is not needed.
    
    inline cort_proto* on_finish(){
        return 0;
    }
    
    void** get_data0(){
        return &data0.result_ptr;
    }
    void** get_data1(){
        return &data1.result_ptr;
    }
};                                                                  

#define CO_GET_NTH_ARG(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N

#define CO_FE_0(co_call, x, ...) co_call(x)
#define CO_FE_1(co_call, x, ...) co_call(x) CO_FE_0(co_call, __VA_ARGS__)
#define CO_FE_2(co_call, x, ...) co_call(x) CO_FE_1(co_call, __VA_ARGS__)
#define CO_FE_3(co_call, x, ...) co_call(x) CO_FE_2(co_call, __VA_ARGS__)
#define CO_FE_4(co_call, x, ...) co_call(x) CO_FE_3(co_call, __VA_ARGS__)
#define CO_FE_5(co_call, x, ...) co_call(x) CO_FE_4(co_call, __VA_ARGS__)
#define CO_FE_6(co_call, x, ...) co_call(x) CO_FE_5(co_call, __VA_ARGS__)
#define CO_FE_7(co_call, x, ...) co_call(x) CO_FE_6(co_call, __VA_ARGS__)
#define CO_FE_8(co_call, x, ...) co_call(x) CO_FE_7(co_call, __VA_ARGS__)
#define CO_FE_9(co_call, x, ...) co_call(x) CO_FE_8(co_call, __VA_ARGS__)

#define CO_FOR_EACH(x, ...) \
    CO_GET_NTH_ARG(__VA_ARGS__, CO_FE_9, CO_FE_8, CO_FE_7, CO_FE_6, CO_FE_5, CO_FE_4, CO_FE_3, CO_FE_2, CO_FE_1, CO_FE_0)(x, __VA_ARGS__)
    
    
// Now let us show an example.
struct cort_example : public cort_proto{

//First put all your context here as class member.
    //int run_times;

//Or you want to overload some member function
    //void clear(){}
    //~cort_example(){}
    //cort_example(){}

//Second using CO_BEGIN to define some useful property
#define CO_BEGIN(cort_example) \
public: \
    typedef cort_example cort_type;\
    typedef cort_proto proto_type;\
    typedef unsigned char count_type;\
    template<typename TTT, unsigned int M = 0>\
    struct cort_state_struct;\
    const static count_type nstate = (count_type)(-1);\
    template<count_type N, int M = 0>                                                           \
    struct struct_int : struct_int<N - 1, 0> {};                                                \
    template<int M>                                                                             \
    struct struct_int<0, M> {};                                                                 \
    static count_type (*first_counter(...))[1];                                                 \
    base_type* start() {                                                                       \
        return  ((cort_state_struct<cort_type>*)(this))->do_exec();                             \
    }                                                                                           \
    cort_type* init() {                                                                         \
        set_run_function((run_type)(&cort_state_struct<cort_type, 0>::do_exec_static));           \
        return  this;                                                                           \
    }                                                                                           \
    struct dummy{       void f(){                                                               \
    CORT_NEXT_STATE(cort_begin)                                                                 
    
#define CORT_NEXT_STATE(cort_state_name) \
    }};const static count_type cort_state_name = CO_STATE_EVAL_COUNTER(first_counter) ;         \
    CO_STATE_INCREASE_COUNTER(first_counter, 1);                                                \
    template<typename CORT_BASE>                                                                        \
    struct cort_state_struct<CORT_BASE, cort_state_name > : public CORT_BASE {                          \
        typedef cort_state_struct<CORT_BASE, cort_state_name > this_type;                               \
        typedef typename CORT_BASE::base_type base_type;                                                \
        const static count_type state_value = cort_state_name;                                          \
        static base_type* do_exec_static(proto_type* this_ptr){return ((this_type*)(this_ptr))->do_exec();}\
        inline base_type* do_exec() { goto ____action_begin; ____action_begin:
        
//Now you can define the coroutine function codes, using the class member as the local variable.

//You can use CO_AWAIT to wait a sub-coroutine. It can not be used in any branch or loop.
#define CO_AWAIT(sub_cort) CO_AWAIT_IMPL(sub_cort, CO_JOIN(CO_STATE_NAME, __LINE__), state_value + 1)

#define CO_AWAIT_ALL(...) do{ \
        size_t current_wait_count = 0; \
        CO_FOR_EACH(CO_AWAIT_MULTI_IMPL, __VA_ARGS__) \
        if(current_wait_count != 0){ \
            this->set_wait_count(current_wait_count); \
            this->set_run_function((run_type)(&cort_state_struct<CORT_BASE, state_value + 1>::do_exec_static)); \
            return this; \
        } \
    }while(false);\
    CO_GOTO_NEXT_STATE \
    CORT_NEXT_STATE(CO_JOIN(CO_STATE_NAME, __LINE__))
    
#define CO_AWAIT_RANGE(sub_cort_begin, sub_cort_end) do{ \
        if(cort_wait_range(this, sub_cort_begin, sub_cort_end) != 0){ \
            this->set_run_function((run_type)(&cort_state_struct<CORT_BASE, state_value + 1>::do_exec_static)); \
            return this; \
        } \
    }while(false); \
    CO_GOTO_NEXT_STATE \
    CORT_NEXT_STATE(CO_JOIN(CO_STATE_NAME, __LINE__))

//After wait finished, it will not turn to next state but current. It behaves like a loop. It can not be used in any branch or loop.
#define CO_AWAIT_BACK(sub_cort) do{ \
        proto_type* __the_sub_cort = (sub_cort)->start();\
        if(__the_sub_cort != 0){\
            __the_sub_cort->set_parent(this); \
            this->set_wait_count(1); \
            this->set_run_function((run_type)(&cort_state_struct<CORT_BASE, state_value>::do_exec_static)); \
            return this; \
        }\
    }while(false); \
    goto ____action_begin; \
    CORT_NEXT_STATE(CO_JOIN(CO_STATE_NAME, __LINE__))

#define CO_AWAIT_MULTI_IMPL(sub_cort) {\
    CO_AWAIT_MULTI_IMPL_IMPL(this, sub_cort) \
}

#define CO_AWAIT_MULTI_IMPL_IMPL(this_ptr, sub_cort) {\
    cort_proto *__the_sub_cort = (sub_cort)->start(); \
    if(__the_sub_cort != 0){ \
        __the_sub_cort->set_parent(this_ptr); \
        ++current_wait_count; \
    }\
}

#define CO_AWAIT_IMPL(sub_cort, cort_state_name, next_state) \
    do{ \
        base_type* __the_sub_cort = (sub_cort)->start();\
        if(__the_sub_cort != 0){\
            __the_sub_cort->set_parent(this); \
            this->set_wait_count(1); \
            this->set_run_function((run_type)(&cort_state_struct<CORT_BASE, next_state>::do_exec_static)); \
            return this; \
        }\
    }while(false); \
    CO_GOTO_NEXT_STATE \
    CORT_NEXT_STATE(cort_state_name)

#define CO_GOTO_NEXT_STATE return ((cort_state_struct<CORT_BASE, state_value + 1>*)(this))->do_exec();

#define CO_AWAIT_AGAIN() do{ \
    this->set_run_function((run_type)(&cort_state_struct<CORT_BASE, state_value>::do_exec_static));  \
    return this; \
}while(false)

#define CO_AWAIT_IF(bool_exp, sub_cort) \
    if(!(bool_exp)){CO_GOTO_NEXT_STATE } \
    CO_AWAIT(sub_cort)  

#define CO_AWAIT_BACK_IF(bool_exp, sub_cort) \
    if(!(bool_exp)){CO_GOTO_NEXT_STATE } \
    CO_AWAIT_BACK(sub_cort)

//Sometimes you want to exit from the couroutine. Using CO_RETURN.
#define CO_RETURN() \
    return this->on_finish(); \

//Sometimes you know you have to pause but you do not know why and when you can continue. 
//Using CO_AWAIT_ANY(), others will use cort_proto::await to tell you what you should wait.
//This is a useful interface for "Dependency Inversion": it enable the coroutine set 
//the resume condition after pause.
#define CO_AWAIT_ANY() do{ \
        this->set_wait_count(0); \
        this->set_run_function((run_type)(&cort_state_struct<CORT_BASE, state_value + 1>::do_exec_static)); \
    }while(false); \
    return this;   \
    CO_GOTO_NEXT_STATE \
    CORT_NEXT_STATE(CO_JOIN(CO_STATE_NAME, __LINE__))

//Sometimes you want to stop the couroutine after a sub_coroutine is finished. Using CO_AWAIT_RETURN.
//It must be used in a branch or loop, or else it must be followed by CO_END.

#define CO_AWAIT_RETURN(sub_cort) \
    do{ \
        base_type* __the_sub_cort = (sub_cort)->start();\
        if(__the_sub_cort != 0){\
            /*__the_sub_cort->set_parent(this->cort_parent); \
            Above codes is not needed */ \
            return __the_sub_cort; \
        }\
        CO_RETURN(); \
    }while(false); 
                                                                                        
#define CO_END(cort_example) CO_RETURN(); }}; \
    const static count_type state_total_count = CO_STATE_EVAL_COUNTER(first_counter) ; 
}; 

#include <iterator>
template <typename T>
size_t cort_wait_range(cort_proto* this_ptr, T begin_forward_iterator, T end_forward_iterator){
    size_t current_wait_count = 0;
    while(begin_forward_iterator != end_forward_iterator){
        typename std::iterator_traits<T>::value_type tmp_cort_new = (*begin_forward_iterator); 
        CO_AWAIT_MULTI_IMPL_IMPL(this_ptr, tmp_cort_new) 
        ++begin_forward_iterator;
    }
    this_ptr->set_wait_count(current_wait_count);
    return current_wait_count;
}

#endif
