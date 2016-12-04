/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "string_utilities.hpp"
#include "debug.hpp"
#include <vector>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
// ntree_node

template<typename T>
class ntree_node
{
public:
  T m_data;
  ntree_node<T>* m_parent;
  std::vector<ntree_node<T>*> m_children;
public:
  ntree_node(const T& d = T()) : m_data(d), m_parent(0)
    {
    }

  ~ntree_node(void)
    {
      m_parent = (ntree_node<T>*)1;
      for (typename std::vector<ntree_node<T>*>::iterator i = m_children.begin(); i != m_children.end(); i++)
        delete *i;
    }

};

template<typename T>
static ntree_node<T>* ntree_copy(ntree_node<T>* root)
{
  if (!root) return 0;
  ntree_node<T>* new_tree = new ntree_node<T>(root->m_data);
  for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
  {
    ntree_node<T>* new_child = ntree_copy(*i);
    new_tree->m_children.push_back(new_child);
    new_child->m_parent = new_tree;
  }
  return new_tree;
}

template<typename T>
static unsigned ntree_size(ntree_node<T>* root)
{
  if (!root) return 0;
  unsigned result = 1;
  for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
    result += ntree_size(*i);
  return result;
}

template<typename T>
static unsigned ntree_depth(ntree_node<T>* root)
{
  unsigned depth = 0;
  for (ntree_node<T>* i = root; i; i = i->m_parent)
    depth++;
  return depth;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
static otext& ntree_print(otext& str, ntree_node<T>* root, unsigned indent)
{
  print_indent(str, indent);
  str << (void*)root;
  if (root)
  {
    str << " {" << (void*)(root->m_parent) << ":";
    for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
    {
      if (i != root->m_children.begin()) str << ",";
      str << (void*)(*i);
    }
    str << "}";
  }
  str << endl;
  if (root)
    for (typename std::vector<ntree_node<T>*>::iterator i = root->m_children.begin(); i != root->m_children.end(); i++)
      ntree_print(str, *i, indent+1);
  return str;
}

////////////////////////////////////////////////////////////////////////////////
// ntree_iterator

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr>::ntree_iterator(void) :
  m_owner(0), m_node(0)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr>::~ntree_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr>::ntree_iterator(const ntree<T>* owner, ntree_node<T>* node) :
  m_owner(owner), m_node(node)
{
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::null (void) const
{
  return m_owner == 0;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::end (void) const
{
  if (m_owner == 0) return false;
  return m_node == 0;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::valid (void) const
{
  return !null() && !end();
}

template<typename T, typename TRef, typename TPtr>
const ntree<T>* ntree_iterator<T,TRef,TPtr>::owner(void) const
{
  return m_owner;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::const_iterator ntree_iterator<T,TRef,TPtr>::constify(void) const
{
  return RETURN_TYPENAME ntree_iterator<T,TRef,TPtr>::const_iterator(m_owner,m_node);
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::iterator ntree_iterator<T,TRef,TPtr>::deconstify(void) const
{
  return RETURN_TYPENAME ntree_iterator<T,TRef,TPtr>::iterator(m_owner,m_node);
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::make_null(void)
{
  m_owner = 0;
  m_node = 0;
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::make_end(void)
{
  m_node = 0;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::operator == (const PARAMETER_TYPENAME ntree_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_node == r.m_node;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_iterator<T,TRef,TPtr>::operator != (const PARAMETER_TYPENAME ntree_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_node != r.m_node;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::reference ntree_iterator<T,TRef,TPtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  chk_valid();
  return m_node->m_data;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_iterator<T,TRef,TPtr>::pointer ntree_iterator<T,TRef,TPtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  chk_valid();
  return &(m_node->m_data);
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::dump(dump_context& context) const throw(persistent_dump_failed)
{
  // dump the magic keys to both the owner pointer and the node pointer
  // if either is not already registered with the dump context, throw an exception
  dump_xref(context,m_owner);
  dump_xref(context,m_node);
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::restore(restore_context& context) throw(persistent_restore_failed)
{
  m_owner = 0;
  m_node = 0;
  restore_xref(context,m_owner);
  restore_xref(context,m_node);
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::chk_owner(const ntree<T>* owner) const
  throw(wrong_object)
{
  if (owner != m_owner)
    throw wrong_object("ntree node iterator");
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::chk_non_null(void) const
  throw(null_dereference)
{
  if (null())
    throw null_dereference("ntree node iterator");
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::chk_non_end(void) const
  throw(end_dereference)
{
  if (end())
    throw end_dereference("ntree node iterator");
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::chk_valid(void) const
  throw(null_dereference,end_dereference)
{
  chk_non_null();
  chk_non_end();
}

template<typename T, typename TRef, typename TPtr>
void ntree_iterator<T,TRef,TPtr>::chk(const ntree<T>* owner) const
  throw(wrong_object,null_dereference,end_dereference)
{
  chk_valid();
  if (owner) chk_owner(owner);
}

template<typename T, typename TRef, typename TPtr>
void dump_ntree_iterator(dump_context& context, const ntree_iterator<T,TRef,TPtr>& data) throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void restore_ntree_iterator(restore_context& context, ntree_iterator<T,TRef,TPtr>& data) throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
// ntree_prefix_iterator

template<typename T, typename TRef, typename TPtr>
ntree_prefix_iterator<T,TRef,TPtr>::ntree_prefix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_prefix_iterator<T,TRef,TPtr>::~ntree_prefix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_prefix_iterator<T,TRef,TPtr>::ntree_prefix_iterator(const ntree_iterator<T,TRef,TPtr>& i) :
  m_iterator(i)
{
  // this is initialised with the root node
  // which is also the first node in prefix traversal order
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::null(void) const
{
  return m_iterator.null();
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::end(void) const
{
  return m_iterator.end();
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::valid(void) const
{
  return m_iterator.valid();
}

template<typename T, typename TRef, typename TPtr>
const ntree<T>* ntree_prefix_iterator<T,TRef,TPtr>::owner(void) const
{
  return m_iterator.owner();
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::const_iterator ntree_prefix_iterator<T,TRef,TPtr>::constify(void) const
{
  return RETURN_TYPENAME ntree_prefix_iterator<T,TRef,TPtr>::const_iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::iterator ntree_prefix_iterator<T,TRef,TPtr>::deconstify(void) const
{
  return RETURN_TYPENAME ntree_prefix_iterator<T,TRef,TPtr>::iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr> ntree_prefix_iterator<T,TRef,TPtr>::simplify(void) const
{
  return ntree_iterator<T,TRef,TPtr>(m_iterator.m_owner,m_iterator.m_node);
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::operator == (const PARAMETER_TYPENAME ntree_prefix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator == r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_prefix_iterator<T,TRef,TPtr>::operator != (const PARAMETER_TYPENAME ntree_prefix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator != r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::this_iterator& ntree_prefix_iterator<T,TRef,TPtr>::operator ++ (void)
  throw(null_dereference,end_dereference)
{
  // pre-increment operator
  // algorithm: if there are any children, visit child 0, otherwise, go to
  // parent and deduce which child the start node was of that parent - if
  // there are further children, go into the next one. Otherwise, go up the
  // tree and test again for further children. Return null if there are no
  // further nodes
  m_iterator.chk_valid();
  if (!m_iterator.m_node->m_children.empty())
  {
    // simply take the first child of this node
    m_iterator.m_node = m_iterator.m_node->m_children[0];
    return *this;
  }
  // this loop walks up the parent pointers
  // either it will walk off the top and exit or a new node will be found and the loop will exit
  for (;;)
  {
    // go up a level
    ntree_node<T>* old_node = m_iterator.m_node;
    m_iterator.m_node = m_iterator.m_node->m_parent;
    // if we've walked off the top of the tree, then return null
    if (!m_iterator.m_node) return *this;
    // otherwise find which index the old node was relative to this node
    typename std::vector<ntree_node<T>*>::iterator found = 
      std::find(m_iterator.m_node->m_children.begin(), m_iterator.m_node->m_children.end(), old_node);
    DEBUG_ASSERT(found != m_iterator.m_node->m_children.end());
    // if this was found, then see if there is another and if so return that
    found++;
    if (found != m_iterator.m_node->m_children.end())
    {
      m_iterator.m_node = *found;
      return *this;
    }
  }
  return *this;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::this_iterator ntree_prefix_iterator<T,TRef,TPtr>::operator ++ (int)
  throw(null_dereference,end_dereference)
{
  // post-increment is defined in terms of the pre-increment
  typename ntree_prefix_iterator<T,TRef,TPtr>::this_iterator result = *this;
  ++(*this);
  return result;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::reference ntree_prefix_iterator<T,TRef,TPtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator*();
}

template<typename T, typename TRef, typename TPtr>
typename ntree_prefix_iterator<T,TRef,TPtr>::pointer ntree_prefix_iterator<T,TRef,TPtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator->();
}

template<typename T, typename TRef, typename TPtr>
void ntree_prefix_iterator<T,TRef,TPtr>::dump(dump_context& context) const throw(persistent_dump_failed)
{
  m_iterator.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void ntree_prefix_iterator<T,TRef,TPtr>::restore(restore_context& context) throw(persistent_restore_failed)
{
  m_iterator.restore(context);
}

template<typename T, typename TRef, typename TPtr>
void dump_ntree_prefix_iterator(dump_context& context, const ntree_prefix_iterator<T,TRef,TPtr>& data) throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void restore_ntree_prefix_iterator(restore_context& context, ntree_prefix_iterator<T,TRef,TPtr>& data) throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
// ntree_postfix_iterator

template<typename T, typename TRef, typename TPtr>
ntree_postfix_iterator<T,TRef,TPtr>::ntree_postfix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_postfix_iterator<T,TRef,TPtr>::~ntree_postfix_iterator(void)
{
}

template<typename T, typename TRef, typename TPtr>
ntree_postfix_iterator<T,TRef,TPtr>::ntree_postfix_iterator(const ntree_iterator<T,TRef,TPtr>& i) :
  m_iterator(i)
{
  // this is initialised with the root node
  // initially traverse to the first node to be visited
  if (m_iterator.m_node)
    while (!m_iterator.m_node->m_children.empty())
      m_iterator.m_node = m_iterator.m_node->m_children[0];
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::null(void) const
{
  return m_iterator.null();
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::end(void) const
{
  return m_iterator.end();
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::valid(void) const
{
  return m_iterator.valid();
}

template<typename T, typename TRef, typename TPtr>
const ntree<T>* ntree_postfix_iterator<T,TRef,TPtr>::owner(void) const
{
  return m_iterator.owner();
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::const_iterator ntree_postfix_iterator<T,TRef,TPtr>::constify(void) const
{
  return RETURN_TYPENAME ntree_postfix_iterator<T,TRef,TPtr>::const_iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::iterator ntree_postfix_iterator<T,TRef,TPtr>::deconstify(void) const
{
  return RETURN_TYPENAME ntree_postfix_iterator<T,TRef,TPtr>::iterator(m_iterator);
}

template<typename T, typename TRef, typename TPtr>
ntree_iterator<T,TRef,TPtr> ntree_postfix_iterator<T,TRef,TPtr>::simplify(void) const
{
  return ntree_iterator<T,TRef,TPtr>(m_iterator.m_owner,m_iterator.m_node);
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::operator == (const PARAMETER_TYPENAME ntree_postfix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator == r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
bool ntree_postfix_iterator<T,TRef,TPtr>::operator != (const PARAMETER_TYPENAME ntree_postfix_iterator<T,TRef,TPtr>::this_iterator& r) const
{
  return m_iterator != r.m_iterator;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::this_iterator& ntree_postfix_iterator<T,TRef,TPtr>::operator ++ (void)
  throw(null_dereference,end_dereference)
{
  // pre-increment operator
  // algorithm: this node has been visited, therefore all children must have
  // already been visited. So go to parent. Return null if the parent is null.
  // Otherwise deduce which child the start node was of that parent - if there
  // are further children, go into the next one and then walk down any
  // subsequent first-child pointers to the bottom. Otherwise, if there are no
  // children then the parent node is the next in the traversal.
  m_iterator.chk_valid();
  if (!m_iterator.m_node) return *this;
  // go up a level
  ntree_node<T>* old_node = m_iterator.m_node;
  m_iterator.m_node = m_iterator.m_node->m_parent;
  // if we've walked off the top of the tree, then the result is null so there's nothing more to be done
  if (!m_iterator.m_node) return *this;
  // otherwise find which index the old node was relative to this node
  typename std::vector<ntree_node<T>*>::iterator found =
    std::find(m_iterator.m_node->m_children.begin(), m_iterator.m_node->m_children.end(), old_node);
  DEBUG_ASSERT(found != m_iterator.m_node->m_children.end());
  // if this was found, then see if there is another - if not then the current node is the next in the iteration
  found++;
  if (found == m_iterator.m_node->m_children.end()) return *this;
  // if so traverse to it
  m_iterator.m_node = *found;
  // now walk down the leftmost child pointers to the bottom of the new sub-tree
  while (!m_iterator.m_node->m_children.empty())
    m_iterator.m_node = m_iterator.m_node->m_children[0];
  return *this;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::this_iterator ntree_postfix_iterator<T,TRef,TPtr>::operator ++ (int)
  throw(null_dereference,end_dereference)
{
  // post-increment is defined in terms of the pre-increment
  typename ntree_postfix_iterator<T,TRef,TPtr>::this_iterator result = *this;
  ++(*this);
  return result;
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::reference ntree_postfix_iterator<T,TRef,TPtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator*();
}

template<typename T, typename TRef, typename TPtr>
typename ntree_postfix_iterator<T,TRef,TPtr>::pointer ntree_postfix_iterator<T,TRef,TPtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  return m_iterator.operator->();
}

template<typename T, typename TRef, typename TPtr>
void ntree_postfix_iterator<T,TRef,TPtr>::dump(dump_context& context) const throw(persistent_dump_failed)
{
  m_iterator.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void ntree_postfix_iterator<T,TRef,TPtr>::restore(restore_context& context) throw(persistent_restore_failed)
{
  m_iterator.restore(context);
}

template<typename T, typename TRef, typename TPtr>
void dump_ntree_postfix_iterator(dump_context& context, const ntree_postfix_iterator<T,TRef,TPtr>& data) throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename T, typename TRef, typename TPtr>
void restore_ntree_postfix_iterator(restore_context& context, ntree_postfix_iterator<T,TRef,TPtr>& data) throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
// ntree

template<typename T>
ntree<T>::ntree(void) : m_root(0)
{
}

template<typename T>
ntree<T>::~ntree(void)
{
  if (m_root) delete m_root;
}

template<typename T>
ntree<T>::ntree(const ntree<T>& r) : m_root(0)
{
  *this = r;
}

template<typename T>
ntree<T>& ntree<T>::operator=(const ntree<T>& r)
{
  if (m_root) delete m_root;
  m_root = ntree_copy(r.m_root);
  return *this;
}

template<typename T>
bool ntree<T>::empty(void) const
{
  return m_root == 0;
}

template<typename T>
unsigned ntree<T>::size(void) const
{
  return ntree_size(m_root);
}

template<typename T>
unsigned ntree<T>::size(const PARAMETER_TYPENAME ntree<T>::const_iterator& i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return ntree_size(i.m_node);
}

template<typename T>
unsigned ntree<T>::size(const PARAMETER_TYPENAME ntree<T>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return ntree_size(i.m_node);
}

template<typename T>
unsigned ntree<T>::depth(const PARAMETER_TYPENAME ntree<T>::const_iterator& i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return ntree_depth(i.m_node);
}

template<typename T>
unsigned ntree<T>::depth(const PARAMETER_TYPENAME ntree<T>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return ntree_depth(i.m_node);
}

template<typename T>
typename ntree<T>::const_iterator ntree<T>::root(void) const
{
  return RETURN_TYPENAME ntree<T>::const_iterator(this,m_root);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::root(void)
{
  return RETURN_TYPENAME ntree<T>::iterator(this,m_root);
}

template<typename T>
unsigned ntree<T>::children(const PARAMETER_TYPENAME ntree<T>::const_iterator& i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return i.m_node->m_children.size();
}

template<typename T>
unsigned ntree<T>::children(const PARAMETER_TYPENAME ntree<T>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return i.m_node->m_children.size();
}

template<typename T>
typename ntree<T>::const_iterator ntree<T>::child(const PARAMETER_TYPENAME ntree<T>::const_iterator& i, unsigned child) const
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.chk(this);
  if (child >= children(i))
    throw std::out_of_range("ntree");
  return RETURN_TYPENAME ntree<T>::const_iterator(this,i.m_node->m_children[child]);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::child(const PARAMETER_TYPENAME ntree<T>::iterator& i, unsigned child)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.chk(this);
  if (child >= children(i))
    throw std::out_of_range("ntree");
  return RETURN_TYPENAME ntree<T>::iterator(this,i.m_node->m_children[child]);
}

template<typename T>
typename ntree<T>::const_iterator ntree<T>::parent(const PARAMETER_TYPENAME ntree<T>::const_iterator& i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return RETURN_TYPENAME ntree<T>::const_iterator(this,i.m_node->m_parent);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::parent(const PARAMETER_TYPENAME ntree<T>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  return RETURN_TYPENAME ntree<T>::iterator(this,i.m_node->m_parent);
}

template<typename T>
typename ntree<T>::const_prefix_iterator ntree<T>::prefix_begin(void) const
{
  return RETURN_TYPENAME ntree<T>::const_prefix_iterator(TEMPORARY_TYPENAME ntree<T>::const_iterator(this,m_root));
}

template<typename T>
typename ntree<T>::prefix_iterator ntree<T>::prefix_begin(void)
{
  return RETURN_TYPENAME ntree<T>::prefix_iterator(TEMPORARY_TYPENAME ntree<T>::iterator(this,m_root));
}

template<typename T>
typename ntree<T>::const_prefix_iterator ntree<T>::prefix_end(void) const
{
  return RETURN_TYPENAME ntree<T>::const_prefix_iterator(TEMPORARY_TYPENAME ntree<T>::const_iterator(this,0));
}

template<typename T>
typename ntree<T>::prefix_iterator ntree<T>::prefix_end(void)
{
  return RETURN_TYPENAME ntree<T>::prefix_iterator(TEMPORARY_TYPENAME ntree<T>::iterator(this,0));
}

template<typename T>
typename ntree<T>::const_postfix_iterator ntree<T>::postfix_begin(void) const
{
  return RETURN_TYPENAME ntree<T>::const_postfix_iterator(TEMPORARY_TYPENAME ntree<T>::const_iterator(this,m_root));
}

template<typename T>
typename ntree<T>::postfix_iterator ntree<T>::postfix_begin(void)
{
  return RETURN_TYPENAME ntree<T>::postfix_iterator(TEMPORARY_TYPENAME ntree<T>::iterator(this,m_root));
}

template<typename T>
typename ntree<T>::const_postfix_iterator ntree<T>::postfix_end(void) const
{
  return RETURN_TYPENAME ntree<T>::const_postfix_iterator(TEMPORARY_TYPENAME ntree<T>::const_iterator(this,0));
}

template<typename T>
typename ntree<T>::postfix_iterator ntree<T>::postfix_end(void)
{
  return RETURN_TYPENAME ntree<T>::postfix_iterator(TEMPORARY_TYPENAME ntree<T>::iterator(this,0));
}

template<typename T>
typename ntree<T>::iterator ntree<T>::insert(const T& data)
{
  // insert a new node as the root
  if (m_root) delete m_root;
  m_root = new ntree_node<T>(data);
  return RETURN_TYPENAME ntree<T>::iterator(this,m_root);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::insert(const PARAMETER_TYPENAME ntree<T>::iterator& i, unsigned offset, const T& data)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  // insert a new node as a child of i
  i.chk(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree_node<T>* new_node = new ntree_node<T>(data);
  i.m_node->m_children.insert(i.m_node->m_children.begin()+offset,new_node);
  new_node->m_parent = i.m_node;
  return RETURN_TYPENAME ntree<T>::iterator(this,new_node);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::append(const PARAMETER_TYPENAME ntree<T>::iterator& i, const T& data)
  throw(wrong_object,null_dereference,end_dereference)
{
  return insert(i, i.m_node->m_children.size(), data);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::insert(const PARAMETER_TYPENAME ntree<T>::iterator& i, unsigned offset, const ntree<T>& tree)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  // insert a whole tree as a child of i
  i.chk(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree_node<T>* new_node = ntree_copy(tree.m_root);
  i.m_node->m_children.insert(i.m_node->m_children.begin()+offset,new_node);
  new_node->m_parent = i.m_node;
  return RETURN_TYPENAME ntree<T>::iterator(this,new_node);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::append(const PARAMETER_TYPENAME ntree<T>::iterator& i, const ntree<T>& tree)
  throw(wrong_object,null_dereference,end_dereference)
{
  return insert(i, children(i), tree);
}

template<typename T>
typename ntree<T>::iterator ntree<T>::push(const PARAMETER_TYPENAME ntree<T>::iterator& node, const T& data)
  throw(wrong_object,null_dereference,end_dereference)
{
  // insert a new node to replace the existing node in the tree
  // making the original node the child of the new node
  // i.e. (node) becomes (new)->(node)
  // afterwards, the iterator still points to the old node, now the child
  // returns the iterator to the new node
  node.chk(this);
  ntree_node<T>* new_node = new ntree_node<T>(data);
  if (node.m_node == m_root)
  {
    // pushing the root node
    m_root = new_node;
    new_node->m_parent = 0;
  }
  else
  {
    // pushing a sub-node
    *(std::find(node.m_node->m_parent->m_children.begin(), node.m_node->m_parent->m_children.end(), node.m_node)) = new_node;
    new_node->m_parent = node.m_node->m_parent;
  }
  // link up the old node as the child of the new node
  new_node->m_children.insert(new_node->m_children.begin(),node.m_node);
  node.m_node->m_parent = new_node;
  return RETURN_TYPENAME ntree<T>::iterator(this,new_node);
}

template<typename T>
void ntree<T>::pop(const PARAMETER_TYPENAME ntree<T>::iterator& parent, unsigned offset)
  throw(wrong_object,null_dereference,end_dereference)
{
  // inverse of push
  // removes the specified child of the parent node, adding its children to the parent node at the same offset
  parent.chk(this);
  ntree_node<T>* node = parent.m_node;
  if (offset >= node->m_children.size())
    throw std::out_of_range("ntree");
  // move the grandchildren first
  ntree_node<T>* child = parent.m_node->m_children[offset];
  while (!child->m_children.empty())
  {
    // remove the last grandchild and insert into node just after the child to be removed
    ntree_node<T>* grandchild = child->m_children[child->m_children.size()-1];
    child->m_children.pop_back();
    node->m_children.insert(node->m_children.begin()+offset+1, grandchild);
    grandchild->m_parent = node;
  }
  // now remove the child
  node->m_children.erase(node->m_children.begin()+offset);
  delete child;
}

template<typename T>
void ntree<T>::erase(void)
{
  // erase the whole tree
  if (m_root) delete m_root;
  m_root = 0;
}

template<typename T>
void ntree<T>::erase(PARAMETER_TYPENAME ntree<T>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  // erase this node and its subtree
  // do this by erasing this child of its parent
  // handle the case of erasing the root
  i.chk(this);
  if (i.m_node == m_root)
  {
    delete m_root;
    m_root = 0;
  }
  else
  {
    DEBUG_ASSERT(i.m_node->m_parent);
    i.m_node->m_parent->m_children.erase(std::find(i.m_node->m_parent->m_children.begin(),
                                                   i.m_node->m_parent->m_children.end(),
                                                   i.m_node));
    delete i.m_node;
  }
  i.make_null();
}

template<typename T>
void ntree<T>::erase(const PARAMETER_TYPENAME ntree<T>::iterator& i, unsigned offset)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.chk(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  delete i.m_node->m_children[offset];
  i.m_node->m_children.erase(i.m_node->m_children.begin() + offset);
}

template<typename T>
ntree<T> ntree<T>::subtree(void)
{
  ntree<T> result;
  result.m_root = ntree_copy(m_root);
  return result;
}

template<typename T>
ntree<T> ntree<T>::subtree(const PARAMETER_TYPENAME ntree<T>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  ntree<T> result;
  result.m_root = ntree_copy(i.m_node);
  return result;
}

template<typename T>
ntree<T> ntree<T>::subtree(const PARAMETER_TYPENAME ntree<T>::iterator& i, unsigned offset)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  // copy the child to form a new subtree
  i.chk(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree<T> result;
  result.m_root = ntree_copy(i.m_node->m_children[offset]);
  return result;
}

template<typename T>
ntree<T> ntree<T>::cut(void)
{
  ntree<T> result;
  result.m_root = m_root;
  m_root = 0;
  return result;
}

template<typename T>
ntree<T> ntree<T>::cut(PARAMETER_TYPENAME ntree<T>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  i.chk(this);
  ntree<T> result;
  if (i.m_node == m_root)
  {
    result.m_root = m_root;
    m_root = 0;
  }
  else
  {
    DEBUG_ASSERT(i.m_node->m_parent);
    typename std::vector<ntree_node<T>*>::iterator found = 
      std::find(i.m_node->m_parent->m_children.begin(), i.m_node->m_parent->m_children.end(), i.m_node);
    DEBUG_ASSERT(found != i.m_node->m_children.end());
    result.m_root = *found;
    i.m_node->m_parent->m_children.erase(found);
  }
  i.make_null();
  if (result.m_root) result.m_root->m_parent = 0;
  return result;
}

template<typename T>
ntree<T> ntree<T>::cut(const PARAMETER_TYPENAME ntree<T>::iterator& i, unsigned offset)
  throw(wrong_object,null_dereference,end_dereference,std::out_of_range)
{
  i.chk(this);
  if (offset > children(i))
    throw std::out_of_range("ntree");
  ntree<T> result;
  result.m_root = i.m_node->m_children[offset];
  i.m_node->m_children.erase(i.m_node->m_children.begin() + offset);
  if (result.m_root) result.m_root->m_parent = 0;
  return result;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
otext& ntree<T>::print(otext& str, unsigned indent) const
{
  return ntree_print(str, m_root, indent);
}

template<typename T>
otext& print_ntree(otext& str, const ntree<T>& tree, unsigned indent)
{
  return tree.print(str, indent);
}

template<typename T>
otext& operator << (otext& str, const ntree<T>& tree)
{
  return print_ntree(str, tree, 0);
}

////////////////////////////////////////////////////////////////////////////////
// persistence

template<typename T>
void dump_ntree_r(dump_context& context, const ntree<T>& tree, const PARAMETER_TYPENAME ntree<T>::const_iterator& node)
  throw(persistent_dump_failed)
{
  // the address of the node is dumped as well as the contents - this is used in iterator persistence
  std::pair<bool,unsigned> node_mapping = context.pointer_map(&(*node));
  if (node_mapping.first) throw persistent_dump_failed("ntree: already dumped this node");
  dump(context,node_mapping.second);
  // now dump the contents
  dump(context,*node);
  /// dump the number of children
  dump(context,tree.children(node));
  // recurse on the children
  for (unsigned i = 0; i < tree.children(node); i++)
    dump_ntree_r(context,tree,tree.child(node,i));
}

template<typename T>
void dump_ntree(dump_context& context, const ntree<T>& tree)
  throw(persistent_dump_failed)
{
  // dump a magic key to the address of the tree for use in persistence of iterators
  // and register it as a dumped address
  std::pair<bool,unsigned> mapping = context.pointer_map(&tree);
  if (mapping.first) throw persistent_dump_failed("ntree: already dumped this tree");
  dump(context,mapping.second);
  // now dump the tree contents
  dump(context, tree.empty());
  if (!tree.empty())
    dump_ntree_r(context,tree,tree.root());
}

template<typename T>
void restore_ntree_r(restore_context& context, ntree<T>& tree, const PARAMETER_TYPENAME ntree<T>::iterator& node)
  throw(persistent_restore_failed)
{
  // restore the node magic key, chk whether it has been used before and add it to the set of known addresses
  unsigned node_magic = 0;
  restore(context,node_magic);
  std::pair<bool,void*> node_mapping = context.pointer_map(node_magic);
  if (node_mapping.first)
    throw persistent_restore_failed("ntree: restored this tree node already");
  context.pointer_add(node_magic,&(*node));
  // now restore the node contents
  restore(context,*node);
  // restore the number of children
  unsigned children = 0;
  restore(context,children);
  // recurse on each child
  for (unsigned i = 0; i < children; i++)
  {
    typename ntree<T>::iterator child = tree.insert(node,i,T());
    restore_ntree_r(context,tree,child);
  }
}

template<typename T>
void restore_ntree(restore_context& context, ntree<T>& tree)
  throw(persistent_restore_failed)
{
  tree.erase();
  // restore the tree's magic key and map it onto the tree's address
  // this is used in the persistence of iterators
  unsigned magic = 0;
  restore(context,magic);
  context.pointer_add(magic,&tree);
  // now restore the contents
  bool empty = true;
  restore(context, empty);
  if (!empty)
  {
    typename ntree<T>::iterator node = tree.insert(T());
    restore_ntree_r(context,tree,node);
  }
}

template<typename T>
void ntree<T>::dump(dump_context& context) const throw(persistent_dump_failed)
{
  dump_ntree(context,*this);
}

template<typename T>
void ntree<T>::restore(restore_context& context) throw(persistent_restore_failed)
{
  restore_ntree(context,*this);
}

////////////////////////////////////////////////////////////////////////////////
