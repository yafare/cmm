// cmm_string_pool.cpp

#include "std_port/std_port_os.h"
#include "cmm_string_pool.h"

namespace cmm
{

StringPool::StringPool()
{
    std_init_spin_lock(&m_lock);
}

StringPool::~StringPool()
{
    for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
        StringImpl::free_string(*it);
    std_destroy_spin_lock(&m_lock);
}
    
// Find string in pool, create new one if not found
// Once a string put in pool, it should be never deleted, so it should be
// a CONSTANT one
StringImpl *StringPool::find_or_insert(const StringPtr& string_ptr)
{
    StringImpl *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find(string_ptr);
    if (it == m_pool.end())
    {
        // Not found in pool, create new "CONSTANT" string
        string_in_pool = StringImpl::alloc_string(string_ptr);
        string_in_pool->attrib |= (ReferenceImpl::CONSTANT | ReferenceImpl::SHARED);
        // Attention: The key (StringPtr) must use the same StringImpl * as value
        m_pool.put(StringPtr(string_in_pool));
    } else
        string_in_pool = (StringImpl *)*it;
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

// Find only
StringImpl *StringPool::find(const StringPtr& string_ptr)
{
    StringImpl *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find(string_ptr);
    if (it != m_pool.end())
        string_in_pool = *it;
    else
        string_in_pool = 0;
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

}
