// mts.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <stdio.h>
#include <stdlib.h>
#include "std_template/simple_hash_map.h"
#include "std_template/simple_list.h"
#include "std_template/simple_string.h"
#include "std_port/std_port.h"
#include "std_port/std_port_type.h"
#include "std_memmgr/std_memmgr.h"
#include "cmm_buffer_new.h"
#include "cmm_domain.h"
#include "cmm_efun.h"
#include "cmm_lang.h"
#include "cmm_lexer.h"
#include "cmm_memory.h"
#include "cmm_object.h"
#include "cmm_program.h"
#include "cmm_thread.h"
#include "cmm_value.h"

#if 1
#include "a_name_2.h"
#include "a_desc_2.h"
#include "a_entity_2.h"
#include "a_desc_i.h"
#endif

long long GetUsCounter()
{
    return std_get_current_us_counter();
}

#define LOCK //lock

namespace std
{

template<>
struct hash<simple::string>
{
    size_t operator()(const simple::string& s) const
    {
        return s.hash_value();
    }
};

}

using namespace cmm;

AutoInitMemoryManager autoInitMemoryManager;

//typedef simple::hash_map<int, int> hmap;
typedef std::unordered_map<int, int> hmap;
long long t;
typedef Integer (*cpp_func_t)(size_t n);
cpp_func_t cpp_func = 0;

#if 0
void IncSum(Test *ob)
{
#if 1
    //hmap table;
    auto thread = Thread::get_current_thread();
    hmap table;
    long long sum = 0;
	cmm::Integer count;
///	Value aaa = new_local_string_value("aaa");
    int to_n = 50;
    int use_cpp = 0;
	for (int i = 0; i < 100; i++)
	{
        if (use_cpp)
        {
            count = cpp_func(to_n);
        }
        else
        {
			count = call(thread, ob, &Test::calc_prime2, to_n).to_int().m_int;
        }
        //        XDELETE(XNEW(int));
        //std::string s("sum");
        //simple::string s("sum");
//        table["sum"]++;
//        table[i] = i;

//        memset(str, 0, sizeof(str));
//        if (memcmp(str, str2, sizeof(str)) != 0)
//            t++;
//        t++;
	}
    t = count;
	//    for (int i = 0; i < 10; i++)
//        table.erase(i);
//    t += sum;
//    if (str[0] != 0)
//        t++;
#endif
}

static void (*fptr)(Test *ob) = IncSum;
IntR threadCount = 0;

void Entry(Test *ob)
{
    long long b, e;
    Thread thread;
    thread.start();
    b = GetUsCounter();
    for (int i = 0; i < 10; i++)
    {
        fptr(ob);
    }
    e = GetUsCounter();
    thread.stop();
    
//    printf("sum = %lld\n", table["sum"]);
    printf("cost = %lld\n", (e - b));
    std_cpu_lock_add(&threadCount, -1);
}
#endif

namespace cmm
{
int conflict = 0;
}

class AAA
{
public:
    AAA(int _n)
    {
        printf("AAA constructor: n = %d\n", _n);
        n = _n;
    }

    AAA() :
        AAA(0)
    {
    }

    ~AAA()
    {
        printf("AAA destructor: n = %d\n", n);
    }

    int n;
};

int main_body(int argn, char *argv[]);

void test_gc()
{
    Uint8 ins[] = { 0x48, 0x89, 0x43, 0x08 };
#if 1
    // Prepare some old mappings
    Value mo = Map();
    printf("mo.m_map = %p\n", mo.m_map);
    for (int i = 0; i < 1000; i++)
    {
        Value mx = Map();
        char str[100];
        snprintf(str, 100, "m_%d", i);
        mo[str] = mx;

        for (int k = 0; k < 100; k++)
        {
            char str[100];
            snprintf(str, 100, "m_%d", i);
            mx[str] = k;
        }
    }
#endif

    auto b = std_get_current_us_counter();
    
    Value m = Map();
    printf("m.m_map = %p\n", m.m_map);
#if 1
    for (int i = 0; i < 1000000; i++)
    {   
        char str[100];
        snprintf(str, 100, "i = %d", i);
        m[str] = str;
    }
#endif
    auto e = std_get_current_us_counter();
    printf("Total Cost = %dus.\n\n", (int)(e - b));
}

void tgc()
{
    typedef simple::vector<int> VVV;
    auto* p = XNEW(MapImpl);
    Value v = Value(p);
    XDELETE(p);
}

int main(int argn, char *argv[])
{
    Value::init();

    Domain::init();
    Object::init();
    Thread::init();
#if 1
    Program::init();
    Simulator::init();
    Efun::init();
    Lang::init();
    Lexer::init();
#endif

    auto* d = Thread::get_current_thread_domain();

    test_gc();
    d->gc();
    d->gc();
////----    getchar();

    ////----    auto *thread = Thread::get_current_thread();
////----    thread->update_start_sp_of_start_domain_context(&argn);

    auto *try_context = Thread::get_current_thread()->get_this_call_context();
////----    try
    {
        auto ret = main_body(argn, argv);
    }
////-----    catch (...)
    {
////----        Thread::get_current_thread()->restore_call_stack_for_error(try_context);
    }

    Lexer::shutdown();
    Lang::shutdown();
    Efun::shutdown();
    Simulator::shutdown();
    Program::shutdown();
    Thread::shutdown();
    Domain::shutdown();
    Object::shutdown();

    Value::shutdown();
}

void ttt();

void compile()
{
    FILE *fp;

    /* Create compile context */
    auto* context = BUFFER_NEW(Lang);

    /* Start new file in lex */ 
    fp = fopen("../script.c", "r");
    if (fp == 0)
        fp = fopen("z:/doing/Project/mts/script.c", "r");
    context->m_lexer.start_new_file(NULL, (IntR)fp, "script.c");
    auto ret = context->parse();
    auto root = context->m_root;

    printf("ret = %d.\n", (int)ret);
    // Debug output
    context->print_ast(context->m_root, 0);

    fclose(fp);
}

int main_body(int argn, char *argv[])
{
    static bool flag = 1;

#if 0
    simple::vector<int> arr;
    arr.push_back(123);
    arr.push_back(456);
    for (auto &it: arr)
       printf("value = %d\n", it);
    return 0;
#endif

#if 0
    simple::hash_map<simple::string, int> m;
    m["123"] = 0;
    m["123"]++;
    printf("%d\n", m["123"]);
    return 0;
#endif

#if 0
    simple::list<int> list;
    list.append(1);
    list.append(3);
    list.append(5);
    list.remove_at(2);
    list.remove_at(1);
    list.remove_at(0);
    for (auto it = list.begin(); it != list.end(); it++)
        printf("%zu. %d\n", it.get_index(), (int) *it);
    return 0;
#endif

    int maxThreadCount = 2;
    if (argn < 2)
	{
		printf("usage: exe n=thread.\nDefault use %d threads.\n", maxThreadCount);
	} else
    {
        maxThreadCount = atoi(argv[1]);
	    if (maxThreadCount < 1)
	    {
		    printf("Bad thread count %d, expected >= 1.\n", maxThreadCount);
		    return -1;
	    }
    }

    compile();

#if 1

#if 1
    Value _v1(UNDEFINED);
    Value _v2(88);
    Value _v3(99.77);
    ObjectId oid;
    oid.i64 = 0x99887766;
    Value _v4(oid);
    Value _v5("abc");
    Value _v6(BUFFER_ALLOC(&maxThreadCount, 8));
    Array _v7(XNEW(ArrayImpl, 3));
    _v7.m_array->push_back("a1");
    _v7.m_array->push_back("b3");
    _v7.m_array->push_back("c7");
    Map _v8(XNEW(MapImpl, 5));
    _v8["name"] = "doing";
    _v8["age"] = 38;
    _v8["gender"] = "male";
    Value vs[] = { _v1, _v2, _v3, _v4, _v5, _v6, _v7, _v8 };

    simple::hash_set<simple::string> set;
    set.put("a");
    set.put("bb");
    set.put("ccc");
#endif

#if 1
    std::string std_str = "std::string";
    auto *buffer1 = BUFFER_NEW(simple::string, "buffer string");
    auto *buffers = BUFFER_NEWN(simple::string, 5);
#endif
//    simple::list<int> list;
#endif

    __clone_entity_ob::create_program();
    __feature_desc_ob::create_program();
    __feature_name_ob::create_program();
    create_desc_i_program();

    Program::update_all_programs();

    auto *thread = Thread::get_current_thread();
    call_efun(thread, "printf", "a=%d\n", 555);

    auto *a1 = BUFFER_NEW(AAA, 888);
    auto *a2 = BUFFER_NEWN(AAA, 3);
    auto *a11 = BUFFER_ALLOC(a1, sizeof(*a1));
    auto *a21 = BUFFER_ALLOC(a2, sizeof(*a2));
    BUFFER_DELETE(a1);
    BUFFER_DELETEN(a2);
    a11->bind_to_current_domain();
    a21->bind_to_current_domain();
    a11 = 0;

    printf("Start GC...\n");
    Thread::get_current_thread_domain()->gc();
    printf("End GC...\n");
    //printf("a1 = %p, a2 = %p, a11 = %p, a21 = %p\n", a1, a2, a11, a21);
    printf("&argn = %p\n", (Uint8*) &argn - 0x20);
    printf("thread start = %p, end = %p\n",
           thread->get_this_domain_context()->value.m_start_sp,
           thread->get_this_domain_context()->value.m_end_sp);

    auto *domain = XNEW(Domain, "test1");
    auto *program = Program::find_program_by_name(Value("/clone/entity").m_string);
    auto *ob = program->new_instance(domain);

    auto *domain2 = XNEW(Domain, "test2");
    auto *ob2 = program->new_instance(domain2);
    call_other(thread, ob->get_oid(), "create");
    call_other(thread, ob2->get_oid(), "create");
    Value ret = call_other(thread, ob->get_oid(), "printf", ob2->get_oid());
//    printf("ret = %d.\n", (int) ret.m_int);
    XDELETE(ob);

    return 0;

#if 0
    simple::vector<Domain *> domains;
    simple::vector<Test *> test_obs;
    domains.push_back(XNEW(Domain));
    test_obs.push_back(XNEW(Test));
    test_obs[0]->set_domain(domains[0]);

    printf("Single thread cost:\n");
//    table["sum"] = 0;
//    table[7] = 1;
#if 1
    threadCount = 1;
    std_create_task(NULL, NULL, (void *)Entry, test_obs[0]);
    while (threadCount > 0)
        std_sleep(1);
#endif

    printf("\nMulti threads cost:\n");
//    table["sum"] = 0;
    for (auto i = 1; i <= maxThreadCount; i++)
    {
        domains.push_back(XNEW(Domain));
        test_obs.push_back(XNEW(Test));
        test_obs[i]->set_domain(domains[0]);
        std_create_task(NULL, NULL, (void *) Entry, test_obs[i]);
    }
    threadCount = maxThreadCount;
    while (threadCount > 0)
        std_sleep(1);
    printf("t = %lld\n", t);
    printf("Conflict = %d\n", conflict);

    // Close all domains
    for (auto i = 0; i < domains.size(); i++)
    {
        domains[i]->gc();
        XDELETE(domains[i]);
        XDELETE(test_obs[i]);
    }
#endif
    return 0;
}
