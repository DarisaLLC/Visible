#ifndef NTREE_HPP
#define NTREE_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  A templated n-ary tree data structure. STL-like but the definition of
  iterators is really only applicable to one-dimensional structures. I use
  iterators to access tree nodes, but there is no increment or decrement
  operators for them. I also define prefix and postfix traversal iterators which
  do have increment.

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "textio.hpp"
#include "persistent.hpp"
#include "exceptions.hpp"
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
// Internals

template<typename T> class ntree_node;
template<typename T> class ntree;
template<typename T, typename TRef, typename TPtr> class ntree_iterator;
template<typename T, typename TRef, typename TPtr> class ntree_prefix_iterator;
template<typename T, typename TRef, typename TPtr> class ntree_postfix_iterator;

////////////////////////////////////////////////////////////////////////////////
// Iterators

// Simple iterators which are just used as pointers to tree nodes. These have
// no increment or decrement operations defined. An uninitialised iterator is
// null - similarly, if you ask for the root of an empty tree or the parent of
// the root node then you get a null iterator.

template<typename T, typename TRef, typename TPtr>
class ntree_iterator
{
public:
  // local type definitions
  // an iterator points to an object whilst a const_iterator points to a const object
  typedef ntree_iterator<T,T&,T*> iterator;
  typedef ntree_iterator<T,const T&,const T*> const_iterator;
  typedef ntree_iterator<T,TRef,TPtr> this_iterator;
  typedef TRef reference;
  typedef TPtr pointer;

  // constructor to create a null iterator - you must assign a valid value to this iterator before using it
  ntree_iterator(void);
  ~ntree_iterator(void);

  // tests
  // a null iterator is one that has not been initialised with a value yet
  // i.e. you just declared it but didn't assign to it
  bool null(void) const;
  // an end iterator is one that points to the end element of the tree of nodes
  // in STL conventions this is one past the last valid element and must not be dereferenced
  bool end(void) const;
  // a valid iterator is one that can be dereferenced
  // i.e. non-null and non-end
  bool valid(void) const;

  // Type conversion methods allow const_iterator and iterator to be converted
  const_iterator constify(void) const;
  iterator deconstify(void) const;

  // make the iterator into a null or end iterator
  void make_null(void);
  void make_end(void);

  // tests useful for putting iterators into other STL structures and for testing whether iteration has completed
  bool operator == (const this_iterator& r) const;
  bool operator != (const this_iterator& r) const;

  // access the node data - a const_iterator gives you a const element, an iterator a non-const element
  // it is illegal to dereference an invalid (i.e. null or end) iterator
  reference operator*(void) const
    throw(null_dereference,end_dereference);
  pointer operator->(void) const
    throw(null_dereference,end_dereference);

  // data persistence
  // Note: the owning ntree must be dumped/restored first
  void dump(dump_context&) const
    throw(persistent_dump_failed);
  void restore(restore_context&)
    throw(persistent_restore_failed);

private:
  friend class ntree<T>;
  friend class ntree_prefix_iterator<T,TRef,TPtr>;
  friend class ntree_postfix_iterator<T,TRef,TPtr>;

  const ntree<T>* m_owner;
  ntree_node<T>* m_node;

  void chk_owner(const ntree<T>* owner) const
    throw(wrong_object);
  void chk_non_null(void) const
    throw(null_dereference);
  void chk_non_end(void) const
    throw(end_dereference);
  void chk_valid(void) const
    throw(null_dereference,end_dereference);
  void chk(const ntree<T>* owner) const
    throw(wrong_object,null_dereference,end_dereference);

public:
  // constructor used by ntree to create a non-null iterator
  // you cannot create a valid iterator except by calling a hash method that returns one
  // Note: I had to make it public to get round a compiler problem - it should be private
  explicit ntree_iterator(const ntree<T>* owner, ntree_node<T>* node);
  const ntree<T>* owner(void) const;
  ntree_node<T>* node(void) const;
};

// non-member versions of the persistence functions

template<typename T, typename TRef, typename TPtr>
void dump_ntree_iterator(dump_context&, const ntree_iterator<T,TRef,TPtr>&)
  throw(persistent_dump_failed);

template<typename T, typename TRef, typename TPtr>
void restore_ntree_iterator(restore_context&, ntree_iterator<T,TRef,TPtr>&)
  throw(persistent_restore_failed);

// Traversal iterators are like iterators but they have increment operators (++)
// - prefix_iterator visits the nodes of the tree in prefix order
// - postfix_iterator visits the nodes of the tree in postfix order.
// There is no such thing as infix order for an n-ary tree and you cannot
// traverse backwards with these iterators. These follow the STL convention in
// that you iterate from a begin to an end - in this case ntree exports
// prefix_begin()/prefix_end() and postfix_begin()/postfix_end(). You can
// simplify these iterators to the basic iterator above for functions that
// require a simple iterator.

template<typename T, typename TRef, typename TPtr>
class ntree_prefix_iterator
{
public:
  typedef ntree_prefix_iterator<T,T&,T*>             iterator;
  typedef ntree_prefix_iterator<T,const T&,const T*> const_iterator;
  typedef ntree_prefix_iterator<T,TRef,TPtr>         this_iterator;
  typedef ntree_iterator<T,TRef,TPtr>                simple_iterator;
  typedef TRef                                       reference;
  typedef TPtr                                       pointer;

  // constructor to create a null iterator - you must assign a valid value to this iterator before using it
  ntree_prefix_iterator(void);
  ~ntree_prefix_iterator(void);

  // tests
  // a null iterator is one that has not been initialised with a value yet
  // i.e. you just declared it but didn't assign to it
  bool null(void) const;
  // an end iterator is one that points to the end element of the list of nodes
  // in STL conventions this is one past the last valid element and must not be dereferenced
  bool end(void) const;
  // a valid iterator is one that can be dereferenced
  // i.e. non-null and non-end
  bool valid(void) const;

  // get the digraph object that created this iterator
  // a null iterator doesn't have an owner so returns a null pointer
  const ntree<T>* owner(void) const;

  // Type conversion methods allow const_iterator and iterator to be converted
  // convert an iterator/const_iterator to a const_iterator
  const_iterator constify(void) const;
  iterator deconstify(void) const;

  // generate a simple iterator from a traversal iterator
  ntree_iterator<T,TRef,TPtr> simplify(void) const;

  // tests useful for putting iterators into other STL structures and for testing whether iteration has completed
  bool operator == (const this_iterator& r) const;
  bool operator != (const this_iterator& r) const;

  // increment/decrement operators used to step through the set of all nodes in a graph
  // it is only legal to increment a valid iterator
  // pre-increment
  this_iterator& operator ++ (void)
    throw(null_dereference,end_dereference);
  // post-increment
  this_iterator operator ++ (int)
    throw(null_dereference,end_dereference);

  // access the node data - a const_iterator gives you a const element, an iterator a non-const element
  // it is illegal to dereference an invalid (i.e. null or end) iterator
  reference operator*(void) const
    throw(null_dereference,end_dereference);
  pointer operator->(void) const
    throw(null_dereference,end_dereference);

  // data persistence
  // Note: the owning ntree must be dumped/restored first
  void dump(dump_context&) const
    throw(persistent_dump_failed);
  void restore(restore_context&)
    throw(persistent_restore_failed);

protected:
  friend class ntree<T>;
  friend class ntree_iterator<T,TRef,TPtr>;

  ntree_iterator<T,TRef,TPtr> m_iterator;

  explicit ntree_prefix_iterator(const ntree_iterator<T,TRef,TPtr>& i);
};

// non-member versions of the persistence functions

template<typename T, typename TRef, typename TPtr>
void dump_ntree_prefix_iterator(dump_context&, const ntree_prefix_iterator<T,TRef,TPtr>&)
  throw(persistent_dump_failed);

template<typename T, typename TRef, typename TPtr>
void restore_ntree_prefix_iterator(restore_context&, ntree_prefix_iterator<T,TRef,TPtr>&)
  throw(persistent_restore_failed);

////////////////////////////////////////////////////////////////////////////////

template<typename T, typename TRef, typename TPtr>
class ntree_postfix_iterator
{
public:
  typedef ntree_postfix_iterator<T,T&,T*>             iterator;
  typedef ntree_postfix_iterator<T,const T&,const T*> const_iterator;
  typedef ntree_postfix_iterator<T,TRef,TPtr>         this_iterator;
  typedef ntree_iterator<T,TRef,TPtr>                 simple_iterator;
  typedef TRef                                        reference;
  typedef TPtr                                        pointer;

  // constructor to create a null iterator - you must assign a valid value to this iterator before using it
  ntree_postfix_iterator(void);
  ~ntree_postfix_iterator(void);

  // tests
  // a null iterator is one that has not been initialised with a value yet
  // i.e. you just declared it but didn't assign to it
  bool null(void) const;
  // an end iterator is one that points to the end element of the list of nodes
  // in STL conventions this is one past the last valid element and must not be dereferenced
  bool end(void) const;
  // a valid iterator is one that can be dereferenced
  // i.e. non-null and non-end
  bool valid(void) const;

  // get the digraph object that created this iterator
  // a null iterator doesn't have an owner so returns a null pointer
  const ntree<T>* owner(void) const;

  // Type conversion methods allow const_iterator and iterator to be converted
  // convert an iterator/const_iterator to a const_iterator
  const_iterator constify(void) const;
  iterator deconstify(void) const;

  // generate a simple iterator from a traversal iterator
  ntree_iterator<T,TRef,TPtr> simplify(void) const;

  // tests useful for putting iterators into other STL structures and for testing whether iteration has completed
  bool operator == (const this_iterator& r) const;
  bool operator != (const this_iterator& r) const;

  // increment/decrement operators used to step through the set of all nodes in a graph
  // it is only legal to increment a valid iterator
  // pre-increment
  this_iterator& operator ++ (void)
    throw(null_dereference,end_dereference);
  // post-increment
  this_iterator operator ++ (int)
    throw(null_dereference,end_dereference);

  // access the node data - a const_iterator gives you a const element, an iterator a non-const element
  // it is illegal to dereference an invalid (i.e. null or end) iterator
  reference operator*(void) const
    throw(null_dereference,end_dereference);
  pointer operator->(void) const
    throw(null_dereference,end_dereference);

  // data persistence
  // Note: the owning ntree must be dumped/restored first
  void dump(dump_context&) const
    throw(persistent_dump_failed);
  void restore(restore_context&)
    throw(persistent_restore_failed);

protected:
  friend class ntree<T>;
  friend class ntree_iterator<T,TRef,TPtr>;

  ntree_iterator<T,TRef,TPtr> m_iterator;

  explicit ntree_postfix_iterator(const ntree_iterator<T,TRef,TPtr>& i);
};

// non-member versions of the persistence functions

template<typename T, typename TRef, typename TPtr>
void dump_ntree_postfix_iterator(dump_context&, const ntree_postfix_iterator<T,TRef,TPtr>&)
  throw(persistent_dump_failed);

template<typename T, typename TRef, typename TPtr>
void restore_ntree_postfix_iterator(restore_context&, ntree_postfix_iterator<T,TRef,TPtr>&)
  throw(persistent_restore_failed);

////////////////////////////////////////////////////////////////////////////////
// The Ntree class
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class ntree
{
public:
  // STL-like typedefs for the types and iterators
  typedef T value_type;

  typedef ntree_iterator<T,T&,T*> iterator;
  typedef ntree_iterator<T,const T&,const T*> const_iterator;

  typedef ntree_prefix_iterator<T,T&,T*> prefix_iterator;
  typedef ntree_prefix_iterator<T,const T&,const T*> const_prefix_iterator;

  typedef ntree_postfix_iterator<T,T&,T*> postfix_iterator;
  typedef ntree_postfix_iterator<T,const T&,const T*> const_postfix_iterator;

  //////////////////////////////////////////////////////////////////////////////
  // Constructors, destructors and copies

  ntree(void);
  ~ntree(void);

  // copy constructor and assignment both copy the tree
  ntree(const ntree<T>&);
  ntree<T>& operator=(const ntree<T>&);
   
  //////////////////////////////////////////////////////////////////////////////
  // size tests

  // tests on whole tree
  bool empty(void) const;
  unsigned size(void) const;

  // tests for number of nodes in subtree starting at node
  unsigned size(const const_iterator& node) const
    throw(wrong_object,null_dereference,end_dereference);
  unsigned size(const iterator& node)
    throw(wrong_object,null_dereference,end_dereference);

  // test for depth of tree from root to node
  unsigned depth(const const_iterator& node) const
    throw(wrong_object,null_dereference,end_dereference);
  unsigned depth(const iterator& node)
    throw(wrong_object,null_dereference,end_dereference);

  //////////////////////////////////////////////////////////////////////////////
  // direct traversal

  const_iterator root(void) const;
  iterator root(void);

  unsigned children(const const_iterator& node) const
    throw(wrong_object,null_dereference,end_dereference);
  unsigned children(const iterator& node)
    throw(wrong_object,null_dereference,end_dereference);

  const_iterator child(const const_iterator& node, unsigned child) const
    throw(wrong_object,null_dereference,end_dereference,std::out_of_range);
  iterator child(const iterator& node, unsigned child)
    throw(wrong_object,null_dereference,end_dereference,std::out_of_range);

  const_iterator parent(const const_iterator& node) const
    throw(wrong_object,null_dereference,end_dereference);
  iterator parent(const iterator& node)
    throw(wrong_object,null_dereference,end_dereference);

  //////////////////////////////////////////////////////////////////////////////
  // iterator traversal

  const_prefix_iterator prefix_begin(void) const;
  prefix_iterator prefix_begin(void);
  const_prefix_iterator prefix_end(void) const;
  prefix_iterator prefix_end(void);

  const_postfix_iterator postfix_begin(void) const;
  postfix_iterator postfix_begin(void);
  const_postfix_iterator postfix_end(void) const;
  postfix_iterator postfix_end(void);

  //////////////////////////////////////////////////////////////////////////////
  // modification

  iterator insert(const T&);

  iterator insert(const iterator& node, unsigned child, const T&)
    throw(wrong_object,null_dereference,end_dereference,std::out_of_range);
  iterator append(const iterator& node, const T&) 
    throw(wrong_object,null_dereference,end_dereference);

  iterator insert(const iterator& node, unsigned child, const ntree<T>&)
    throw(wrong_object,null_dereference,end_dereference,std::out_of_range);
  iterator append(const iterator& node, const ntree<T>&)
    throw(wrong_object,null_dereference,end_dereference);

  iterator push(const iterator& node, const T&) 
    throw(wrong_object,null_dereference,end_dereference);
  void pop(const iterator& node, unsigned child) 
    throw(wrong_object,null_dereference,end_dereference);

  void erase(void);
  void erase(iterator& node)
    throw(wrong_object,null_dereference,end_dereference);
  void erase(const iterator& node, unsigned child)
    throw(wrong_object,null_dereference,end_dereference,std::out_of_range);

  ntree<T> subtree(void);
  ntree<T> subtree(const iterator& node)
    throw(wrong_object,null_dereference,end_dereference);
  ntree<T> subtree(const iterator& node, unsigned child)
    throw(wrong_object,null_dereference,end_dereference,std::out_of_range);

  ntree<T> cut(void);
  ntree<T> cut(iterator& node)
    throw(wrong_object,null_dereference,end_dereference);
  ntree<T> cut(const iterator& node, unsigned child)
    throw(wrong_object,null_dereference,end_dereference,std::out_of_range);

  //////////////////////////////////////////////////////////////////////////////
  // Diagnostic printout

  otext& print(otext& str, unsigned indent = 0) const;

  //////////////////////////////////////////////////////////////////////////////
  // persistence
  
  void dump(dump_context&) const
    throw(persistent_dump_failed);
  void restore(restore_context&)
    throw(persistent_restore_failed);

  //////////////////////////////////////////////////////////////////////////////

protected:
  ntree_node<T>* m_root;
};

////////////////////////////////////////////////////////////////////////////////
// Non-member version of print

template<typename T> otext& print_ntree(otext& str, const ntree<T>& tree, unsigned indent = 0);
template<typename T> otext& operator << (otext& str, const ntree<T>& tree);

////////////////////////////////////////////////////////////////////////////////
// persistence functions

template<typename T>
void dump_ntree(dump_context& str, const ntree<T>& data)
  throw(persistent_dump_failed);
template<typename T>
void restore_ntree(restore_context& str, ntree<T>& data)
  throw(persistent_restore_failed);

////////////////////////////////////////////////////////////////////////////////

#include "ntree.tpp"
#endif
