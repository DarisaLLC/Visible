


/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Note: I tried to write this using STL lists for the node and arc lists, but
  it got far too hairy. The specific problem is that I wanted a digraph
  iterator to contain a list::iterator so I needed to be able to generate a
  list::iterator from a node or arc and STL list iterators don't give you that
  functionality. I tried burgling the data structures, but that was
  non-portable between different STL implementations so needed lots of #ifdefs
  and so was mind-bogglingly awful and unreadable - in other words a
  maintenance nightmare. I gave up and impemented my own lists - not difficult.

  I use circular double-linked lists with a dummy node to represent the list
  end() or root. This is the node pointed to by the graph itself. The circular
  design means that both ends of the list are equally accessible in unit time.
  This design also means that there is always an end node in the list which is
  required to implement STL-like iterator semantics where end() represents the
  element after the last real element even when the list is empty. It also
  means that the list is always fully linked - i.e. the next and previous
  pointers of every node are always pointing to something so I never need to
  chk for a null pointer.

  *** Special Feature in this week's edition: ***
  Compiler problems encountered when writing this:

  (1) The recursive functions, like most recursions, are implemented as an
  exported 'seed' function which starts the recursion and a 'work' function
  which actually does the recursion but usually has an ugly interface and is
  not meant to be called from anywhere but the 'seed' function. These 'work'
  functions were originally made non-members so that I could hide them from the
  class interface. Unfortunately, M***o$oft VC++ crashes with a fatal compiler
  error (C1001 if you're interested. No? Don't blame you) if you do this. My
  guess is that calling a non-member template from a member template freaks it
  out. I believe this is only a problem with templates. Anyway, I made these
  functions members and it works. It means the header has been slightly
  uglified by the presence of declarations for these 'work' functions but at
  least I've made them private so no dumb-f*** tries to call them.

  (2) I failed to find a way of declaring the bodies of template friend
  functions. I had this problem with gcc (my primary development compiler) and
  never got round to trying it with VC++. There's no point trying if it won't
  port to gcc anyway. The obvious solution was to simply move these functions
  outside the class. However, I could not find a way in which template
  functions outside the class could access typedefs declared in the class (such
  as the path_vector type) so I couldn't work it that way either. This may only
  be a problem with gcc, but the same comment above about porting between
  compilers applies.

  ------------------------------------------------------------------------------*/
#include "string_utilities.hpp"
#include "debug.hpp"
#include <algorithm>
#include <deque>

////////////////////////////////////////////////////////////////////////////////
// Internals

template<typename NT, typename AT> class digraph_node
{
public:
  NT data;
  digraph_node<NT,AT>* m_prev;
  digraph_node<NT,AT>* m_next;
  std::vector<digraph_arc<NT,AT>*> m_inputs;
  std::vector<digraph_arc<NT,AT>*> m_outputs;
public:
  digraph_node(const NT& d = NT()) :
    data(d), m_prev(0), m_next(0) {}
};

template<typename NT, typename AT> class digraph_arc
{
public:
  AT data;
  digraph_arc<NT,AT>* m_prev;
  digraph_arc<NT,AT>* m_next;
  digraph_node<NT,AT>* m_from;
  digraph_node<NT,AT>* m_to;
  digraph_arc(digraph_node<NT,AT>* f = 0, digraph_node<NT,AT>* t = 0, const AT& d = AT()) : 
    data(d), m_prev(0), m_next(0), m_from(f), m_to(t) {}
};

////////////////////////////////////////////////////////////////////////////////
// Iterators

// Node iterator

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_iterator<NT,AT,NRef,NPtr>::chk_owner(const digraph<NT,AT>* owner) const
  throw(wrong_object)
{
  if (owner != m_owner)
    throw wrong_object("digraph node iterator");
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_iterator<NT,AT,NRef,NPtr>::chk_non_null(void) const
  throw(null_dereference)
{
  if (null())
    throw null_dereference("digraph node iterator");
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_iterator<NT,AT,NRef,NPtr>::chk_non_end(void) const
  throw(end_dereference)
{
  if (end())
    throw end_dereference("digraph node iterator");
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_iterator<NT,AT,NRef,NPtr>::chk_valid(void) const
  throw(null_dereference,end_dereference)
{
  chk_non_null();
  chk_non_end();
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_iterator<NT,AT,NRef,NPtr>::chk(const digraph<NT,AT>* owner) const
  throw(wrong_object,null_dereference,end_dereference)
{
  chk_valid();
  if (owner) chk_owner(owner);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
digraph_iterator<NT,AT,NRef,NPtr>::digraph_iterator(void) :
  m_owner(0), m_node(0)
{
}

template<typename NT, typename AT, typename NRef, typename NPtr>
digraph_iterator<NT,AT,NRef,NPtr>::~digraph_iterator(void)
{
}

template<typename NT, typename AT, typename NRef, typename NPtr>
digraph_iterator<NT,AT,NRef,NPtr>::digraph_iterator(const digraph<NT,AT>* owner, digraph_node<NT,AT>* node) : 
  m_owner(owner), m_node(node)
{
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_iterator<NT,AT,NRef,NPtr>::null (void) const
{
  return m_node == 0;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_iterator<NT,AT,NRef,NPtr>::end (void) const
{
  if (m_node == 0) return false;
  return m_node == m_owner->m_nodes;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_iterator<NT,AT,NRef,NPtr>::valid (void) const
{
  return !null() && !end();
}

template<typename NT, typename AT, typename NRef, typename NPtr>
const digraph<NT,AT>* digraph_iterator<NT,AT,NRef,NPtr>::owner(void) const
{
  return m_owner;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::const_iterator digraph_iterator<NT,AT,NRef,NPtr>::constify (void) const
{
  return RETURN_TYPENAME digraph_iterator<NT,AT,NRef,NPtr>::const_iterator(m_owner,m_node);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::iterator digraph_iterator<NT,AT,NRef,NPtr>::deconstify (void) const
{
  return RETURN_TYPENAME digraph_iterator<NT,AT,NRef,NPtr>::iterator(m_owner,m_node);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::this_iterator& digraph_iterator<NT,AT,NRef,NPtr>::operator ++ (void)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  m_node = m_node->m_next;
  return *this;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::this_iterator digraph_iterator<NT,AT,NRef,NPtr>::operator ++ (int)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  digraph_iterator<NT,AT,NRef,NPtr> old = *this;
  m_node = m_node->m_next;
  return old;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::this_iterator& digraph_iterator<NT,AT,NRef,NPtr>::operator -- (void)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  m_node = m_node->m_prev;
  return *this;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::this_iterator digraph_iterator<NT,AT,NRef,NPtr>::operator -- (int)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  digraph_iterator<NT,AT,NRef,NPtr> old = *this;
  m_node = m_node->m_prev;
  return old;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_iterator<NT,AT,NRef,NPtr>::operator == (const PARAMETER_TYPENAME digraph_iterator<NT,AT,NRef,NPtr>::this_iterator& r) const
{
  return m_node == r.m_node;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_iterator<NT,AT,NRef,NPtr>::operator != (const PARAMETER_TYPENAME digraph_iterator<NT,AT,NRef,NPtr>::this_iterator& r) const
{
  return m_node != r.m_node;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_iterator<NT,AT,NRef,NPtr>::operator < (const PARAMETER_TYPENAME digraph_iterator<NT,AT,NRef,NPtr>::this_iterator& r) const
{
  return m_node < r.m_node;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::reference digraph_iterator<NT,AT,NRef,NPtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  chk_valid();
  return m_node->data;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_iterator<NT,AT,NRef,NPtr>::pointer digraph_iterator<NT,AT,NRef,NPtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  chk_valid();
  return &(m_node->data);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_iterator<NT,AT,NRef,NPtr>::dump(dump_context& context) const
  throw(persistent_dump_failed)
{
  // dump the magic keys to both the owner pointer and the node pointer
  // if either is not already registered with the dump context, throw an exception
  dump_xref(context,m_owner);
  dump_xref(context,m_node);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_iterator<NT,AT,NRef,NPtr>::restore(restore_context& context)
  throw(persistent_restore_failed)
{
  m_owner = 0;
  m_node = 0;
  restore_xref(context,m_owner);
  restore_xref(context,m_node);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void dump_digraph_iterator(dump_context& context, const digraph_iterator<NT,AT,NRef,NPtr>& data)
  throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void restore_digraph_iterator(restore_context& context, digraph_iterator<NT,AT,NRef,NPtr>& data)
  throw(persistent_restore_failed)
{
  data.restore(context);
}

// Arc Iterator

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_arc_iterator<NT,AT,NRef,NPtr>::chk_owner(const digraph<NT,AT>* owner) const
  throw(wrong_object)
{
  if (owner != m_owner)
    throw wrong_object("digraph arc iterator");
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_arc_iterator<NT,AT,NRef,NPtr>::chk_non_null(void) const
  throw(null_dereference)
{
  if (null())
    throw null_dereference("digraph arc iterator");
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_arc_iterator<NT,AT,NRef,NPtr>::chk_non_end(void) const
  throw(end_dereference)
{
  if (end())
    throw end_dereference("digraph arc iterator");
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_arc_iterator<NT,AT,NRef,NPtr>::chk_valid(void) const
  throw(null_dereference,end_dereference)
{
  chk_non_null();
  chk_non_end();
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_arc_iterator<NT,AT,NRef,NPtr>::chk(const digraph<NT,AT>* owner) const
  throw(wrong_object,null_dereference,end_dereference)
{
  chk_valid();
  if (owner) chk_owner(owner);
}

template<typename NT, typename AT, typename ARef, typename APtr>
digraph_arc_iterator<NT,AT,ARef,APtr>::digraph_arc_iterator(void) :
  m_owner(0),m_arc(0)
{
}

template<typename NT, typename AT, typename ARef, typename APtr>
digraph_arc_iterator<NT,AT,ARef,APtr>::~digraph_arc_iterator(void)
{
}

template<typename NT, typename AT, typename ARef, typename APtr>
digraph_arc_iterator<NT,AT,ARef,APtr>::digraph_arc_iterator(const digraph<NT,AT>* owner, digraph_arc<NT,AT>* arc) :
  m_owner(owner), m_arc(arc)
{
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_arc_iterator<NT,AT,NRef,NPtr>::null (void) const
{
  return m_arc == 0;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_arc_iterator<NT,AT,NRef,NPtr>::end (void) const
{
  if (m_arc == 0) return false;
  return m_arc == m_owner->m_arcs;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
bool digraph_arc_iterator<NT,AT,NRef,NPtr>::valid (void) const
{
  return !null() && !end();
}

template<typename NT, typename AT, typename NRef, typename NPtr>
const digraph<NT,AT>* digraph_arc_iterator<NT,AT,NRef,NPtr>::owner(void) const
{
  return m_owner;
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_arc_iterator<NT,AT,NRef,NPtr>::const_iterator digraph_arc_iterator<NT,AT,NRef,NPtr>::constify (void) const
{
  return RETURN_TYPENAME digraph_arc_iterator<NT,AT,NRef,NPtr>::const_iterator(m_owner, m_arc);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
typename digraph_arc_iterator<NT,AT,NRef,NPtr>::iterator digraph_arc_iterator<NT,AT,NRef,NPtr>::deconstify (void) const
{
  return RETURN_TYPENAME digraph_arc_iterator<NT,AT,NRef,NPtr>::iterator(m_owner, m_arc);
}

template<typename NT, typename AT, typename ARef, typename APtr>
typename digraph_arc_iterator<NT,AT,ARef,APtr>::this_iterator& digraph_arc_iterator<NT,AT,ARef,APtr>::operator ++ (void)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  m_arc = m_arc->m_next;
  return *this;
}

template<typename NT, typename AT, typename ARef, typename APtr>
typename digraph_arc_iterator<NT,AT,ARef,APtr>::this_iterator digraph_arc_iterator<NT,AT,ARef,APtr>::operator ++ (int)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  digraph_arc_iterator<NT,AT,ARef,APtr> old = *this;
  m_arc = m_arc->m_next;
  return old;
}

template<typename NT, typename AT, typename ARef, typename APtr>
typename digraph_arc_iterator<NT,AT,ARef,APtr>::this_iterator& digraph_arc_iterator<NT,AT,ARef,APtr>::operator -- (void)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  m_arc = m_arc->m_prev;
  return *this;
}

template<typename NT, typename AT, typename ARef, typename APtr>
typename digraph_arc_iterator<NT,AT,ARef,APtr>::this_iterator digraph_arc_iterator<NT,AT,ARef,APtr>::operator -- (int)
  throw(null_dereference,end_dereference)
{
  chk_valid();
  digraph_arc_iterator<NT,AT,ARef,APtr> old = *this;
  m_arc = m_arc->m_prev;
  return old;
}

template<typename NT, typename AT, typename ARef, typename APtr>
bool digraph_arc_iterator<NT,AT,ARef,APtr>::operator == (const PARAMETER_TYPENAME digraph_arc_iterator<NT,AT,ARef,APtr>::this_iterator& r) const
{
  return m_arc == r.m_arc;
}

template<typename NT, typename AT, typename ARef, typename APtr>
bool digraph_arc_iterator<NT,AT,ARef,APtr>::operator != (const PARAMETER_TYPENAME digraph_arc_iterator<NT,AT,ARef,APtr>::this_iterator& r) const
{
  return m_arc != r.m_arc;
}

template<typename NT, typename AT, typename ARef, typename APtr>
bool digraph_arc_iterator<NT,AT,ARef,APtr>::operator < (const PARAMETER_TYPENAME digraph_arc_iterator<NT,AT,ARef,APtr>::this_iterator& r) const
{
  return m_arc < r.m_arc;
}

template<typename NT, typename AT, typename ARef, typename APtr>
typename digraph_arc_iterator<NT,AT,ARef,APtr>::reference digraph_arc_iterator<NT,AT,ARef,APtr>::operator*(void) const
  throw(null_dereference,end_dereference)
{
  chk_valid();
  return m_arc->data;
}

template<typename NT, typename AT, typename ARef, typename APtr>
typename digraph_arc_iterator<NT,AT,ARef,APtr>::pointer digraph_arc_iterator<NT,AT,ARef,APtr>::operator->(void) const
  throw(null_dereference,end_dereference)
{
  chk_valid();
  return &(m_arc->data);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_arc_iterator<NT,AT,NRef,NPtr>::dump(dump_context& context) const
  throw(persistent_dump_failed)
{
  dump_xref(context,m_owner);
  dump_xref(context,m_arc);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void digraph_arc_iterator<NT,AT,NRef,NPtr>::restore(restore_context& context)
  throw(persistent_restore_failed)
{
  m_owner = 0;
  m_arc = 0;
  restore_xref(context,m_owner);
  restore_xref(context,m_arc);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void dump_digraph_arc_iterator(dump_context& context, const digraph_arc_iterator<NT,AT,NRef,NPtr>& data)
  throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename NT, typename AT, typename NRef, typename NPtr>
void restore_digraph_arc_iterator(restore_context& context, digraph_arc_iterator<NT,AT,NRef,NPtr>& data)
  throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
// subtype utilities

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_vector digraph<NT,AT>::constify_arcs(const PARAMETER_TYPENAME digraph<NT,AT>::arc_vector& arcs) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::const_arc_vector result;
  for (unsigned i = 0; i < arcs.size(); i++)
  {
    arcs[i].chk(this);
    result.push_back(arcs[i].constify());
  }
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_vector digraph<NT,AT>::deconstify_arcs(const PARAMETER_TYPENAME digraph<NT,AT>::const_arc_vector& arcs) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::arc_vector result;
  for (unsigned i = 0; i < arcs.size(); i++)
  {
    arcs[i].chk(this);
    result.push_back(arcs[i].deconstify());
  }
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_path_vector digraph<NT,AT>::constify_paths(const PARAMETER_TYPENAME digraph<NT,AT>::path_vector& paths) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::const_path_vector result;
  for (unsigned i = 0; i < paths.size(); i++)
    result.push_back(constify_arcs(paths[i]));
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::path_vector digraph<NT,AT>::deconstify_paths(const PARAMETER_TYPENAME digraph<NT,AT>::const_path_vector& paths) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::path_vector result;
  for (unsigned i = 0; i < paths.size(); i++)
    result.push_back(deconstify_arcs(paths[i]));
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_node_vector digraph<NT,AT>::constify_nodes(const PARAMETER_TYPENAME digraph<NT,AT>::node_vector& nodes) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::const_node_vector result;
  for (unsigned i = 0; i < nodes.size(); i++)
  {
    nodes[i].chk(this);
    result.push_back(nodes[i].constify());
  }
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::node_vector digraph<NT,AT>::deconstify_nodes(const PARAMETER_TYPENAME digraph<NT,AT>::const_node_vector& nodes) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::node_vector result;
  for (unsigned i = 0; i < nodes.size(); i++)
  {
    nodes[i].chk(this);
    result.push_back(nodes[i].deconstify());
  }
  return result;
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::npos(void)
{
  return(unsigned)-1;
}

////////////////////////////////////////////////////////////////////////////////
// Constructors etc.

template<typename NT, typename AT>
digraph<NT,AT>::digraph(void)
{
  // the node and arc lists have a dummy node which acts as the end node
  // they are circular double-linked lists so that next and previous pointers are never null
  // initially they point to themselves to represent a empty lists
  m_nodes = new digraph_node<NT,AT>;
  m_nodes->m_next = m_nodes;
  m_nodes->m_prev = m_nodes;
  m_arcs = new digraph_arc<NT,AT>;
  m_arcs->m_next = m_arcs;
  m_arcs->m_prev = m_arcs;
}

template<typename NT, typename AT>
digraph<NT,AT>::~digraph(void)
{
  // delete all but the dummy nodes
  clear();
  // now delete the dummy nodes
  delete m_nodes;
  m_nodes = 0;
  delete m_arcs;
  m_arcs = 0;
}
   
template<typename NT, typename AT>
digraph<NT,AT>::digraph(const digraph<NT,AT>& r)
{
  m_nodes = new digraph_node<NT,AT>;
  m_nodes->m_next = m_nodes;
  m_nodes->m_prev = m_nodes;
  m_arcs = new digraph_arc<NT,AT>;
  m_arcs->m_next = m_arcs;
  m_arcs->m_prev = m_arcs;
  *this = r;
}

template<typename NT, typename AT>
digraph<NT,AT>& digraph<NT,AT>::operator=(const digraph<NT,AT>& r)
{
  // make it self-copy safe i.e. a=a; is a valid instruction
  if (this == &r) return *this;
  clear();
  // first phase is to copy the nodes, creating a map of cross references from the old nodes to their new equivalents
  std::map<TEMPLATE_TYPENAME digraph<NT,AT>::const_iterator, TEMPLATE_TYPENAME digraph<NT,AT>::iterator> xref;
  for (typename digraph<NT,AT>::const_iterator n = r.begin(); n != r.end(); n++)
    xref[n] = insert(*n);
  // second phase is to copy the arcs, using the map to convert the old to and from nodes to the new nodes
  for (typename digraph<NT,AT>::const_arc_iterator a = r.arc_begin(); a != r.arc_end(); a++)
    arc_insert(xref[r.arc_from(a)],xref[r.arc_to(a)],*a);
  return *this;
}
   
////////////////////////////////////////////////////////////////////////////////
// Basic Node functions

template<typename NT, typename AT>
bool digraph<NT,AT>::empty(void) const
{
  return m_nodes->m_next == m_nodes;
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::size(void) const
{
  unsigned count = 0;
  for (typename digraph<NT,AT>::const_iterator i = begin(); i != end(); i++)
    count++;
  return count;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::iterator digraph<NT,AT>::insert(const NT& node_data)
{
  // insert at the end of the list
  digraph_node<NT,AT>* new_node = new digraph_node<NT,AT>(node_data);
  new_node->m_next = m_nodes;
  new_node->m_prev = m_nodes->m_prev;
  m_nodes->m_prev->m_next = new_node;
  m_nodes->m_prev = new_node;
  return RETURN_TYPENAME digraph<NT,AT>::iterator(this,new_node);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::iterator digraph<NT,AT>::erase(PARAMETER_TYPENAME digraph<NT,AT>::iterator iter)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  // don't allow end to be erased because that is the dummy node that holds the list to the graph
  // this could throw an exception
  if (iter == end()) return end();
  // remove all arcs connected to this node first
  // use arc_erase rather than arcs.erase because that tidies up the node at the other end of the arc too
  for (unsigned i = fanin(iter); i--; )
    arc_erase(input(iter,i));
  for (unsigned j = fanout(iter); j--; )
    arc_erase(output(iter,j));
  // now unlink the node from the list and delete it
  iter.m_node->m_next->m_prev = iter.m_node->m_prev;
  iter.m_node->m_prev->m_next = iter.m_node->m_next;
  digraph_node<NT,AT>* m_next = iter.m_node->m_next;
  delete iter.m_node;
  // return the next node in the list
  return RETURN_TYPENAME digraph<NT,AT>::iterator(this,m_next);
}

template<typename NT, typename AT>
void digraph<NT,AT>::clear(void)
{
  // clearing the nodes also clears the arcs
  for (typename digraph<NT,AT>::iterator i = begin(); i != end(); )
    i = erase(i);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_iterator digraph<NT,AT>::begin(void) const
{
  return RETURN_TYPENAME digraph<NT,AT>::const_iterator(this,m_nodes->m_next);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::iterator digraph<NT,AT>::begin(void)
{
  return RETURN_TYPENAME digraph<NT,AT>::iterator(this,m_nodes->m_next);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_iterator digraph<NT,AT>::end(void) const
{
  return RETURN_TYPENAME digraph<NT,AT>::const_iterator(this,m_nodes);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::iterator digraph<NT,AT>::end(void)
{
  return RETURN_TYPENAME digraph<NT,AT>::iterator(this,m_nodes);
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::fanin(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator iter) const
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return iter.m_node->m_inputs.size();
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::fanin(PARAMETER_TYPENAME digraph<NT,AT>::iterator iter)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return iter.m_node->m_inputs.size();
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_iterator digraph<NT,AT>::input(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator iter, unsigned i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::const_arc_iterator(this,iter.m_node->m_inputs[i]);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_iterator digraph<NT,AT>::input(PARAMETER_TYPENAME digraph<NT,AT>::iterator iter, unsigned i)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::arc_iterator(this,iter.m_node->m_inputs[i]);
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::fanout(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator iter) const
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return iter.m_node->m_outputs.size();
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::fanout(PARAMETER_TYPENAME digraph<NT,AT>::iterator iter)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return iter.m_node->m_outputs.size();
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_iterator digraph<NT,AT>::output(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator iter, unsigned i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::const_arc_iterator(this,iter.m_node->m_outputs[i]);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_iterator digraph<NT,AT>::output(PARAMETER_TYPENAME digraph<NT,AT>::iterator iter, unsigned i)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::arc_iterator(this,iter.m_node->m_outputs[i]);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_vector digraph<NT,AT>::inputs(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator node) const
  throw(wrong_object,null_dereference,end_dereference)
{
  node.chk(this);
  typename digraph<NT,AT>::const_arc_vector result;
  for (unsigned i = 0; i < fanin(node); i++)
    result.push_back(input(node,i));
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_vector digraph<NT,AT>::inputs(PARAMETER_TYPENAME digraph<NT,AT>::iterator node)
  throw(wrong_object,null_dereference,end_dereference)
{
  node.chk(this);
  typename digraph<NT,AT>::arc_vector result;
  for (unsigned i = 0; i < fanin(node); i++)
    result.push_back(input(node,i));
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_vector digraph<NT,AT>::outputs(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator node) const
  throw(wrong_object,null_dereference,end_dereference)
{
  node.chk(this);
  typename digraph<NT,AT>::const_arc_vector result;
  for (unsigned i = 0; i < fanout(node); i++)
    result.push_back(output(node,i));
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_vector digraph<NT,AT>::outputs(PARAMETER_TYPENAME digraph<NT,AT>::iterator node)
  throw(wrong_object,null_dereference,end_dereference)
{
  node.chk(this);
  typename digraph<NT,AT>::arc_vector result;
  for (unsigned i = 0; i < fanout(node); i++)
    result.push_back(output(node,i));
  return result;
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::output_offset(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                       PARAMETER_TYPENAME digraph<NT,AT>::const_arc_iterator arc) const
  throw(wrong_object,null_dereference,end_dereference)
{
  from.chk(this);
  arc.chk(this);
  for (unsigned i = 0; i < fanout(from); i++)
  {
    if (output(from,i) == arc)
      return i;
  }
  return digraph<NT,AT>::npos();
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::output_offset(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                                       PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator arc)
  throw(wrong_object,null_dereference,end_dereference)
{
  from.chk(this);
  arc.chk(this);
  for (unsigned i = 0; i < fanout(from); i++)
  {
    if (output(from,i) == arc)
      return i;
  }
  return digraph<NT,AT>::npos();
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::input_offset(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to,
                                      PARAMETER_TYPENAME digraph<NT,AT>::const_arc_iterator arc) const
  throw(wrong_object,null_dereference,end_dereference)
{
  to.chk(this);
  arc.chk(this);
  for (unsigned i = 0; i < fanin(to); i++)
  {
    if (input(to,i) == arc)
      return i;
  }
  return digraph<NT,AT>::npos();
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::input_offset(PARAMETER_TYPENAME digraph<NT,AT>::iterator to,
                                      PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator arc)
  throw(wrong_object,null_dereference,end_dereference)
{
  to.chk(this);
  arc.chk(this);
  for (unsigned i = 0; i < fanin(to); i++)
  {
    if (input(to,i) == arc)
      return i;
  }
  return digraph<NT,AT>::npos();
}

////////////////////////////////////////////////////////////////////////////////
// Basic Arc functions

template<typename NT, typename AT>
bool digraph<NT,AT>::arc_empty(void) const
{
  return m_arcs->m_next == m_arcs;
}

template<typename NT, typename AT>
unsigned digraph<NT,AT>::arc_size(void) const
{
  unsigned count = 0;
  for (typename digraph<NT,AT>::const_arc_iterator i = arc_begin(); i != arc_end(); i++)
    count++;
  return count;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_iterator digraph<NT,AT>::arc_insert(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                                                                 PARAMETER_TYPENAME digraph<NT,AT>::iterator to,
                                                                 const AT& arc_data)
  throw(wrong_object,null_dereference,end_dereference)
{
  from.chk(this);
  to.chk(this);
  // create the new arc and link it in to the arc list
  digraph_arc<NT,AT>* new_arc = new digraph_arc<NT,AT>(from.m_node, to.m_node, arc_data);
  new_arc->m_next = m_arcs;
  new_arc->m_prev = m_arcs->m_prev;
  m_arcs->m_prev->m_next = new_arc;
  m_arcs->m_prev = new_arc;
  // add this arc to the inputs and outputs of the end nodes
  from.m_node->m_outputs.push_back(new_arc);
  to.m_node->m_inputs.push_back(new_arc);
  return RETURN_TYPENAME digraph<NT,AT>::arc_iterator(this,new_arc);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_iterator digraph<NT,AT>::arc_erase(PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator iter)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  // first remove this arc's pointers from the from/to nodes
  for (typename std::vector<digraph_arc<NT,AT>*>::iterator i = iter.m_arc->m_to->m_inputs.begin(); i != iter.m_arc->m_to->m_inputs.end(); )
  {
    if (*i == iter.m_arc)
      i = iter.m_arc->m_to->m_inputs.erase(i);
    else
      i++;
  }
  for (typename std::vector<digraph_arc<NT,AT>*>::iterator o = iter.m_arc->m_from->m_outputs.begin(); o != iter.m_arc->m_from->m_outputs.end(); )
  {
    if (*o == iter.m_arc)
      o = iter.m_arc->m_from->m_outputs.erase(o);
    else
      o++;
  }
  // now unlink the arc from the list and delete it
  iter.m_arc->m_next->m_prev = iter.m_arc->m_prev;
  iter.m_arc->m_prev->m_next = iter.m_arc->m_next;
  digraph_arc<NT,AT>* m_next = iter.m_arc->m_next;
  delete iter.m_arc;
  return RETURN_TYPENAME digraph<NT,AT>::arc_iterator(this,m_next);
}

template<typename NT, typename AT>
void digraph<NT,AT>::arc_clear(void)
{
  for (typename digraph<NT,AT>::arc_iterator a = arc_begin(); a != arc_end(); )
    a = arc_erase(a);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_iterator digraph<NT,AT>::arc_begin(void) const
{
  return RETURN_TYPENAME digraph<NT,AT>::const_arc_iterator(this,m_arcs->m_next);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_iterator digraph<NT,AT>::arc_begin(void)
{
  return RETURN_TYPENAME digraph<NT,AT>::arc_iterator(this,m_arcs->m_next);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_iterator digraph<NT,AT>::arc_end(void) const
{
  return RETURN_TYPENAME digraph<NT,AT>::const_arc_iterator(this,m_arcs);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_iterator digraph<NT,AT>::arc_end(void)
{
  return RETURN_TYPENAME digraph<NT,AT>::arc_iterator(this,m_arcs);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_iterator digraph<NT,AT>::arc_from(PARAMETER_TYPENAME digraph<NT,AT>::const_arc_iterator iter) const
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::const_iterator(this,iter.m_arc->m_from);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::iterator digraph<NT,AT>::arc_from(PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator iter)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::iterator(this,iter.m_arc->m_from);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_iterator digraph<NT,AT>::arc_to(PARAMETER_TYPENAME digraph<NT,AT>::const_arc_iterator iter) const
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::const_iterator(this,iter.m_arc->m_to);
}

template<typename NT, typename AT>
typename digraph<NT,AT>::iterator digraph<NT,AT>::arc_to(PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator iter)
  throw(wrong_object,null_dereference,end_dereference)
{
  iter.chk(this);
  return RETURN_TYPENAME digraph<NT,AT>::iterator(this,iter.m_arc->m_to);
}

template<typename NT, typename AT>
void digraph<NT,AT>::arc_move(PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator arc,
                              PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                              PARAMETER_TYPENAME digraph<NT,AT>::iterator to)
  throw(wrong_object,null_dereference,end_dereference)
{
  arc_move_to(arc,to);
  arc_move_from(arc,from);
}

template<typename NT, typename AT>
void digraph<NT,AT>::arc_move_from(PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator arc,
                                   PARAMETER_TYPENAME digraph<NT,AT>::iterator from)
  throw(wrong_object,null_dereference,end_dereference)
{
  arc.chk(this);
  from.chk(this);
  for (typename std::vector<digraph_arc<NT,AT>*>::iterator o = arc.m_arc->m_from->m_outputs.begin(); o != arc.m_arc->m_from->m_outputs.end(); )
  {
    if (*o == arc.m_arc)
      o = arc.m_arc->m_from->m_outputs.erase(o);
    else
      o++;
  }
  from.m_node->m_outputs.push_back(arc.m_arc);
  arc.m_arc->m_from = from.m_node;
}

template<typename NT, typename AT>
void digraph<NT,AT>::arc_move_to(PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator arc,
                                 PARAMETER_TYPENAME digraph<NT,AT>::iterator to)
  throw(wrong_object,null_dereference,end_dereference)
{
  arc.chk(this);
  to.chk(this);
  for (typename std::vector<digraph_arc<NT,AT>*>::iterator i = arc.m_arc->m_to->m_inputs.begin(); i != arc.m_arc->m_to->m_inputs.end(); )
  {
    if (*i == arc.m_arc)
      i = arc.m_arc->m_to->m_inputs.erase(i);
    else
      i++;
  }
  to.m_node->m_inputs.push_back(arc.m_arc);
  arc.m_arc->m_to = to.m_node;
}

template<typename NT, typename AT>
void digraph<NT,AT>::arc_flip(PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator arc)
  throw(wrong_object,null_dereference,end_dereference)
{
  arc_move(arc,arc_to(arc),arc_from(arc));
}

////////////////////////////////////////////////////////////////////////////////
// Adjacency Algorithms

template<typename NT, typename AT>
bool digraph<NT,AT>::adjacent(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                              PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to) const
  throw(wrong_object,null_dereference,end_dereference)
{
  return adjacent_arc(from,to) != arc_end();
}

template<typename NT, typename AT>
bool digraph<NT,AT>::adjacent(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                              PARAMETER_TYPENAME digraph<NT,AT>::iterator to)
  throw(wrong_object,null_dereference,end_dereference)
{
  return adjacent_arc(from,to) != arc_end();
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_iterator digraph<NT,AT>::adjacent_arc(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                                                         PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to) const
  throw(wrong_object,null_dereference,end_dereference)
{
  from.chk(this);
  to.chk(this);
  for (unsigned arc = 0; arc < fanout(from); arc++)
  {
    if (arc_to(output(from, arc)) == to)
      return output(from,arc);
  }
  return arc_end();
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_iterator digraph<NT,AT>::adjacent_arc(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                                                                   PARAMETER_TYPENAME digraph<NT,AT>::iterator to)
  throw(wrong_object,null_dereference,end_dereference)
{
  return adjacent_arc(from.constify(), to.constify()).deconstify();
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_vector digraph<NT,AT>::adjacent_arcs(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                                                         PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to) const
  throw(wrong_object,null_dereference,end_dereference)
{
  from.chk(this);
  to.chk(this);
  typename digraph<NT,AT>::const_arc_vector result;
  for (unsigned arc = 0; arc < fanout(from); arc++)
  {
    if (arc_to(output(from, arc)) == to)
      result.push_back(output(from,arc));
  }
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_vector digraph<NT,AT>::adjacent_arcs(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                                                                   PARAMETER_TYPENAME digraph<NT,AT>::iterator to)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_arcs(adjacent_arcs(from.constify(), to.constify()));
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_node_vector digraph<NT,AT>::input_adjacencies(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::const_node_vector result;
  for (unsigned arc = 0; arc < fanin(to); arc++)
  {
    typename digraph<NT,AT>::const_iterator from = arc_from(input(to, arc));
    if (std::find(result.begin(), result.end(), from) == result.end())
      result.push_back(from);
  }
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::node_vector digraph<NT,AT>::input_adjacencies(PARAMETER_TYPENAME digraph<NT,AT>::iterator to)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_nodes(input_adjacencies(to.constify()));
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_node_vector digraph<NT,AT>::output_adjacencies(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::const_node_vector result;
  for (unsigned arc = 0; arc < fanout(from); arc++)
  {
    typename digraph<NT,AT>::const_iterator to = arc_to(output(from, arc));
    if (find(result.begin(), result.end(), to) == result.end())
      result.push_back(to);
  }
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::node_vector digraph<NT,AT>::output_adjacencies(PARAMETER_TYPENAME digraph<NT,AT>::iterator from)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_nodes(output_adjacencies(from.constify()));
}

////////////////////////////////////////////////////////////////////////////////
// Topographical Sort Algorithms

template<typename NT, typename AT>
std::pair<TEMPLATE_TYPENAME digraph<NT,AT>::const_node_vector, TEMPLATE_TYPENAME digraph<NT,AT>::const_arc_vector>
  digraph<NT,AT>::sort(PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
{
  typename digraph<NT,AT>::const_node_vector result;
  typename digraph<NT,AT>::const_arc_vector errors;
  // build a map containing the number of fanins to each node that must be visited before this one
  typename std::map<TEMPLATE_TYPENAME digraph<NT,AT>::const_iterator,unsigned> fanin_map;
  for (typename digraph<NT,AT>::const_iterator n = begin(); n != end(); n++)
  {
    unsigned predecessors = 0;
    // only count predecessors connected by selected arcs
    for (unsigned f = 0; f < fanin(n); f++)
    {
      typename digraph<NT,AT>::const_arc_iterator input_arc = input(n,f);
      typename digraph<NT,AT>::const_iterator predecessor = arc_from(input_arc);
      if (!select || select(*this,input_arc))
        predecessors++;
    }
    if (predecessors == 0)
    {
      result.push_back(n);
    }
    else
    {
      fanin_map[n] = predecessors;
    }
  }
  // main algorithm applies the topographical sort repeatedly. For a DAG, it
  // will complete first time. However, with backward arcs, the first
  // iteration will fail. The algorithm then tries breaking random arcs to try
  // to get an ordering.
  for(unsigned i = 0; !fanin_map.empty(); )
  {
    // now visit each node in traversal order, decrementing the fanin count of
    // all successors. As each successor's fanin count goes to zero, it is
    // appended to the result.
    for (; i < result.size(); i++)
    {
      // Note: dereferencing i gives us a node iterator
      typename digraph<NT,AT>::const_iterator current = result[i];
      for (unsigned f = 0; f < fanout(current); f++)
      {
        // only consider successors connected by selected arcs
        typename digraph<NT,AT>::const_arc_iterator output_arc = output(current, f);
        typename digraph<NT,AT>::const_iterator successor = arc_to(output_arc);
        if (!select || select(*this,output_arc))
        {
          // don't consider arcs that have been eliminated to break a loop
          if (fanin_map.find(successor) != fanin_map.end())
          {
            --fanin_map[successor];
            if ((fanin_map[successor]) == 0)
            {
              result.push_back(successor);
              fanin_map.erase(fanin_map.find(successor));
            }
          }
        }
      }
    }
    if (!fanin_map.empty())
    {
      // there must be backward arcs preventing completion
      // try removing arcs from the sort to get a partial ordering containing all the nodes

      // select an arc that is still relevant to the sort and break it
      // first select a node that has non-zero fanin and its predecessor that has non-zero fanin
      typename digraph<NT,AT>::const_iterator stuck_node = fanin_map.begin()->first;
      for (unsigned f = 0; f < fanin(stuck_node); f++)
      {
        // now successively remove input arcs that are still part of the sort until the fanin reduces to zero
        // first find a relevant arc - this must be a selected arc that has not yet been traversed by the first half of the algorithm
        typename digraph<NT,AT>::const_arc_iterator input_arc = input(stuck_node, f);
        if (!select || select(*this,input_arc))
        {
          typename digraph<NT,AT>::const_iterator predecessor = arc_from(input_arc);
          if (fanin_map.find(predecessor) != fanin_map.end())
          {
            // found the right combination - remove this arc and then drop out of the fanin loop to restart the outer sort loop
            errors.push_back(input_arc);
            --fanin_map[stuck_node];
            if ((fanin_map[stuck_node]) == 0)
            {
              result.push_back(stuck_node);
              fanin_map.erase(fanin_map.find(stuck_node));
              break;
            }
          }
        }
      }
    }
  }
  return make_pair(result,errors);
}

template<typename NT, typename AT>
std::pair<TEMPLATE_TYPENAME digraph<NT,AT>::node_vector, TEMPLATE_TYPENAME digraph<NT,AT>::arc_vector>
  digraph<NT,AT>::sort(PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
{
  std::pair<TEMPLATE_TYPENAME digraph<NT,AT>::const_node_vector, TEMPLATE_TYPENAME digraph<NT,AT>::const_arc_vector> const_result =
    const_cast<const digraph<NT,AT>*>(this)->sort(select);
  std::pair<TEMPLATE_TYPENAME digraph<NT,AT>::node_vector,TEMPLATE_TYPENAME digraph<NT,AT>::arc_vector> result =
    make_pair(deconstify_nodes(const_result.first),deconstify_arcs(const_result.second));
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_node_vector digraph<NT,AT>::dag_sort(PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
{
  std::pair<TEMPLATE_TYPENAME digraph<NT,AT>::const_node_vector, TEMPLATE_TYPENAME digraph<NT,AT>::const_arc_vector> result = sort(select);
  if (result.second.empty()) return result.first;
  return RETURN_TYPENAME digraph<NT,AT>::const_node_vector();
}

template<typename NT, typename AT>
typename digraph<NT,AT>::node_vector digraph<NT,AT>::dag_sort(PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
{
  return deconstify_nodes(const_cast<const digraph<NT,AT>*>(this)->dag_sort(select));
}
////////////////////////////////////////////////////////////////////////////////
// Path Algorithms

template<typename NT, typename AT>
bool digraph<NT,AT>::path_exists_r(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                   PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to,
                                   PARAMETER_TYPENAME digraph<NT,AT>::const_iterator_set& visited,
                                   PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // Recursive part of the digraph::path_exists function. This is based on a
  // depth first search algorithm and stops the moment it finds a path
  // regardless of its length. Simply traverse every output and recurse on that
  // node until we find the to node or run out of things to recurse on. However,
  // to avoid infinite recursion due to cycles in the graph, I need to maintain
  // a set of visited nodes. The visited set is updated when a candidate is
  // found but tested before the recursion on the candidate so that the number of
  // function calls is minimised.
  for (unsigned i = 0; i < fanout(from); i++)
  {
    typename digraph<NT,AT>::const_arc_iterator arc = output(from,i);
    if (!select || select(*this, arc))
    {
      typename digraph<NT,AT>::const_iterator node = arc_to(arc);
      // if the node is the target, return immediately
      if (node == to) return true;
      // update the visited set and give up if the insert fails, which indicates that the node has already been visited
      if (!(visited.insert(node).second)) return false;
      // now recurse - a path exists from from to to if a path exists from an adjacent node to to
      if (path_exists_r(node,to,visited,select)) return true;
    }
  }
  return false;
}

template<typename NT, typename AT>
bool digraph<NT,AT>::path_exists(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                 PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to, 
                                 PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // set up the recursion with its initial visited set and then recurse
  typename digraph<NT,AT>::const_iterator_set visited;
  visited.insert(from);
  return path_exists_r(from, to, visited, select);
}

template<typename NT, typename AT>
bool digraph<NT,AT>::path_exists(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                                 PARAMETER_TYPENAME digraph<NT,AT>::iterator to,
                                 PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
  throw(wrong_object,null_dereference,end_dereference)
{
  return path_exists(from.constify(), to.constify(), select);
}

template<typename NT, typename AT>
void digraph<NT,AT>::all_paths_r(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                 PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to,
                                 PARAMETER_TYPENAME digraph<NT,AT>::const_arc_vector& so_far,
                                 PARAMETER_TYPENAME digraph<NT,AT>::const_path_vector& result,
                                 PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // This is the recursive part of the all_paths function. The field so_far
  // contains the path so far so that when 'to' is reached, the path is
  // complete. It serves the same purpose as the visited set in the path_exists
  // function except that it also preserves the path order. It also serves the
  // purpose of detecting cycles and thus stopping infinite recursion. Every
  // time the recursion reaches the to node, a copy of so_far is appended to the
  // path set.
  for (unsigned i = 0; i < fanout(from); i++)
  {
    typename digraph<NT,AT>::const_arc_iterator candidate = output(from,i);
    // chk that the arc is selected and then chk that the candidate has not
    // been visited on this path and only allow further recursion if it hasn't
    if ((!select || select(*this, candidate)) && std::find(so_far.begin(), so_far.end(), candidate) == so_far.end())
    {
      // extend the path tracing the route to this arc
      so_far.push_back(candidate);
      // if the candidate arc points to the target, update the result set and prevent further recursion, otherwise recurse
      if (arc_to(candidate) == to)
        result.push_back(so_far);
      else
        all_paths_r(arc_to(candidate),to,so_far,result,select);
      so_far.pop_back();
    }
  }
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_path_vector digraph<NT,AT>::all_paths(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from, 
                                                                     PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to,
                                                                     PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // set up the recursion with empty data fields and then recurse
  typename digraph<NT,AT>::const_path_vector result;
  typename digraph<NT,AT>::const_arc_vector so_far;
  all_paths_r(from, to, so_far, result, select);
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::path_vector digraph<NT,AT>::all_paths(PARAMETER_TYPENAME digraph<NT,AT>::iterator from, 
                                                               PARAMETER_TYPENAME digraph<NT,AT>::iterator to,
                                                               PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_paths(all_paths(from.constify(), to.constify(), select));
}

template<typename NT, typename AT>
void digraph<NT,AT>::reachable_nodes_r(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                       PARAMETER_TYPENAME digraph<NT,AT>::const_iterator_set& visited,
                                       PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // The recursive part of the reachable_nodes function.
  // This is a depth-first traversal again but this time it carries on to find all the reachable nodes
  // Just keep recursing on all the adjacent nodes of each node, skipping already visited nodes to avoid cycles
  for (unsigned i = 0; i < fanout(from); i++)
  {
    typename digraph<NT,AT>::const_arc_iterator arc = output(from,i);
    if (!select || select(*this,arc))
    {
      typename digraph<NT,AT>::const_iterator candidate = arc_to(arc);
      if (visited.insert(candidate).second)
        reachable_nodes_r(candidate,visited,select);
    }
  }
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_node_vector digraph<NT,AT>::reachable_nodes(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                                                           PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // seed the recursion, marking the starting node as already visited
  typename digraph<NT,AT>::const_iterator_set visited;
  visited.insert(from);
  reachable_nodes_r(from, visited, select);
  // convert the visited set into the required output form
  // exclude the starting node
  typename digraph<NT,AT>::const_node_vector result;
  for (typename digraph<NT,AT>::const_iterator_set_iterator i = visited.begin(); i != visited.end(); i++)
    if (*i != from)
      result.push_back(*i);
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::node_vector digraph<NT,AT>::reachable_nodes(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                                                                     PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_nodes(reachable_nodes(from.constify(), select));
}

template<typename NT, typename AT>
void digraph<NT,AT>::reaching_nodes_r(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to,
                                      PARAMETER_TYPENAME digraph<NT,AT>::const_iterator_set& visited,
                                      PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // The recursive part of the reaching_nodes function.
  // Just like the reachable_nodes_r function but it goes backwards
  for (unsigned i = 0; i < fanin(to); i++)
  {
    typename digraph<NT,AT>::const_arc_iterator arc = input(to,i);
    if (!select || select(*this,arc))
    {
      typename digraph<NT,AT>::const_iterator candidate = arc_from(input(to,i));
      if (visited.insert(candidate).second)
        reaching_nodes_r(candidate,visited,select);
    }
  }
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_node_vector digraph<NT,AT>::reaching_nodes(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to,
                                                                          PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // seed the recursion, marking the starting node as already visited
  typename digraph<NT,AT>::const_iterator_set visited;
  visited.insert(to);
  reaching_nodes_r(to,visited,select);
  // convert the visited set into the required output form
  // exclude the end node
  typename digraph<NT,AT>::const_node_vector result;
  for (typename digraph<NT,AT>::const_iterator_set_iterator i = visited.begin(); i != visited.end(); i++)
    if (*i != to)
      result.push_back(*i);
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::node_vector digraph<NT,AT>::reaching_nodes(PARAMETER_TYPENAME digraph<NT,AT>::iterator to,
                                                                    PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_nodes(reaching_nodes(to.constify(),select));
}

////////////////////////////////////////////////////////////////////////////////
// Shortest Path Algorithms

template<typename NT, typename AT>
typename digraph<NT,AT>::const_arc_vector digraph<NT,AT>::shortest_path(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                                                        PARAMETER_TYPENAME digraph<NT,AT>::const_iterator to,
                                                                        PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  typename digraph<NT,AT>::const_path_vector paths = all_paths(from,to,select);
  typename digraph<NT,AT>::const_arc_vector shortest;
  typedef TYPEDEF_TYPENAME digraph<NT,AT>::const_path_vector::iterator const_path_vector_iterator;
  for (const_path_vector_iterator i = paths.begin(); i != paths.end(); i++)
    if (shortest.empty() || i->size() < shortest.size())
      shortest = *i;
  return shortest;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::arc_vector digraph<NT,AT>::shortest_path(PARAMETER_TYPENAME digraph<NT,AT>::iterator from, 
                                                                  PARAMETER_TYPENAME digraph<NT,AT>::iterator to,
                                                                  PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_arcs(shortest_path(from.constify(),to.constify(),select));
}

template<typename NT, typename AT>
typename digraph<NT,AT>::const_path_vector digraph<NT,AT>::shortest_paths(PARAMETER_TYPENAME digraph<NT,AT>::const_iterator from,
                                                                          PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select) const
  throw(wrong_object,null_dereference,end_dereference)
{
  from.chk(this);
  // This is an unweighted shortest path algorithm based on the algorithm from
  // Weiss's book. This is essentially a breadth-first traversal or graph
  // colouring algorithm. It is an iterative algorithm, so no recursion here! It
  // works by creating a node queue initialised with the starting node. It then
  // consumes the queue from front to back. For each node, it finds the
  // successors and appends them to the queue. If a node is already 'known' it
  // is not added - this avoids cycles. Thus the queue insert ordering
  // represents the breadth-first ordering. On the way it creates a map of
  // visited nodes. This is a map not a set because it also stores the arc that
  // nominated this node as a shortest path. The full path can then be recreated
  // from the map by just walking back through the predecessors. The depth (or
  // colour) can be determined by the path length.
  typename digraph<NT,AT>::const_path_vector result;
  // initialise the iteration by creating a queue and adding the start node
  typename std::deque<TEMPLATE_TYPENAME digraph<NT,AT>::const_iterator> nodes;
  nodes.push_back(from);
  // Create a map to store the set of known nodes mapped to their predecessor
  // arcs. Initialise it with the current node, which has no predecessor. Note
  // that the algorithm uses the feature of digraph iterators that they can be
  // null iterators and that all null iterators are equal.
  typedef std::map<TEMPLATE_TYPENAME digraph<NT,AT>::const_iterator, TEMPLATE_TYPENAME digraph<NT,AT>::const_arc_iterator> known_map;
  known_map known;
  known.insert(make_pair(from,typename digraph<NT,AT>::const_arc_iterator()));
  // now the iterative part of the algorithm
  while(!nodes.empty())
  {
    // pop the queue to get the next node to process - unfortunately the STL deque::pop does not return the popped value
    typename digraph<NT,AT>::const_iterator current = nodes.front();
    nodes.pop_front();
    // now visit all the successors
    for (unsigned i = 0; i < fanout(current); i++)
    {
      typename digraph<NT,AT>::const_arc_iterator next_arc = output(current,i);
      // chk whether the successor arc is a selected arc and can be part of a path
      if (!select || select(*this,next_arc))
      {
        typename digraph<NT,AT>::const_iterator next = arc_to(next_arc);
        // Discard any successors that are known because to be known already they
        // must have another shorter path. Otherwise add the successor node to the
        // queue to be visited later. To minimise the overhead of map lookup I use
        // the usual trick of trying to insert the node and determining whether
        // the node was known by the success or failure of the insertion - this is
        // a Good STL Trick (TM).
        if (known.insert(make_pair(next,next_arc)).second)
          nodes.push_back(next);
      }
    }
  }
  // The map contains the results as an unordered set of nodes, mapped to their
  // predecessor arcs and weight. This now needs to be converted into a set of
  // paths. This is done by starting with a node from the map, finding its
  // predecessor arc and therefore its predecessor node, looking that up in the
  // map to find its predecessor and so on until the start node is reached (it
  // has a null predecessor). Note that the known set includes the from node
  // which does not generate a path.
  for (typename known_map::iterator i = known.begin(); i != known.end(); i++)
  {
    if (i->first != from)
    {
      const_arc_vector this_path;
      for (typename known_map::iterator node = i; 
           node->second != typename digraph<NT,AT>::const_arc_iterator(); 
           node = known.find(arc_from(node->second)))
        this_path.insert(this_path.begin(),node->second);
      result.push_back(this_path);
    }
  }
  return result;
}

template<typename NT, typename AT>
typename digraph<NT,AT>::path_vector digraph<NT,AT>::shortest_paths(PARAMETER_TYPENAME digraph<NT,AT>::iterator from,
                                                                    PARAMETER_TYPENAME digraph<NT,AT>::arc_select_fn select)
  throw(wrong_object,null_dereference,end_dereference)
{
  return deconstify_paths(shortest_paths(from.constify(),select));
}

////////////////////////////////////////////////////////////////////////////////
// Print functions

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_node(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::const_iterator& i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  return str << "node:" << to_string((void*)&*i,16,radix_none);
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_node(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  return str << "node:" << to_string((void*)&*i,16,radix_none);
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_arc(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::const_arc_iterator& i) const
  throw(wrong_object,null_dereference,end_dereference)
{
  return str << "arc:" << to_string((void*)&*i,16,radix_none);
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_arc(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::arc_iterator& i)
  throw(wrong_object,null_dereference,end_dereference)
{
  return str << "arc:" << to_string((void*)&*i,16,radix_none);
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_arc_vector(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::const_arc_vector& arcs) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // inline format is:
  //   {a,b,c}
  str << "{";
  typedef TYPEDEF_TYPENAME digraph<NT,AT>::const_arc_vector::const_iterator const_arc_vector_const_iterator;
  for (const_arc_vector_const_iterator i = arcs.begin(); i != arcs.end(); i++)
  {
    i->chk(this);
    if (i != arcs.begin()) str << ",";
    print_arc(str, *i);
  }
  str << "}";
  return str;
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_arc_vector(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::arc_vector& arcs)
  throw(wrong_object,null_dereference,end_dereference)
{
  return print_arc_vector(str, constify_arcs(arcs));
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_path_vector(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::const_path_vector& paths) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // format: path,path,path
  typedef TYPEDEF_TYPENAME digraph<NT,AT>::const_path_vector::const_iterator const_path_vector_const_iterator;
  for (const_path_vector_const_iterator i = paths.begin(); i != paths.end(); i++)
  {
    if (i != paths.begin()) str << ",";
    print_arc_vector(str, *i);
  }
  return str;
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_path_vector(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::path_vector& paths)
  throw(wrong_object,null_dereference,end_dereference)
{
  return print_path_vector(str, constify_paths(paths));
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_node_vector(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::const_node_vector& nodes) const
  throw(wrong_object,null_dereference,end_dereference)
{
  // inline format is:
  //   {a,b,c}
  str << "{";
  typedef TYPEDEF_TYPENAME digraph<NT,AT>::const_node_vector::const_iterator const_node_vector_const_iterator;
  for (const_node_vector_const_iterator i = nodes.begin(); i != nodes.end(); i++)
  {
    i->chk(this);
    if (i != nodes.begin()) str << ",";
    print_node(str, *i);
  }
  str << "}";
  return str;
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print_node_vector(otext& str, const PARAMETER_TYPENAME digraph<NT,AT>::node_vector& nodes)
  throw(wrong_object,null_dereference,end_dereference)
{
  return print_node_vector(str, constify_nodes(nodes));
}

template<typename NT, typename AT>
otext& digraph<NT,AT>::print(otext& str, unsigned indent) const
{
  print_indent(str,indent);
  str << "nodes:" << endl;
  for (typename digraph<NT,AT>::const_iterator n = begin(); n != end(); ++n)
  {
    print_indent(str,indent+1);
    print_node(str, n);
    str << " inputs: ";
    print_arc_vector(str, inputs(n));
    str << " outputs: ";
    print_arc_vector(str, outputs(n));
    str << endl;
  }
  print_indent(str,indent);
  str << "arcs:" << endl;
  for (typename digraph<NT,AT>::const_arc_iterator a = arc_begin(); a != arc_end(); ++a)
  {
    print_indent(str,indent+1);
    print_arc(str, a);
    str << " from ";
    print_node(str, arc_from(a));
    str << " to ";
    print_node(str, arc_to(a));
    str << endl;
  }
  return str;
}

////////////////////////////////////////////////////////////////////////////////
// Non-member print functions

template<typename NT, typename AT>
otext& print_graph(otext& str, const digraph<NT,AT>& graph, unsigned indent)
{
  return graph.print(str,indent);
}

template<typename NT, typename AT>
otext& operator << (otext& str, const digraph<NT,AT>& graph)
{
  return print_graph(str,graph);
}

////////////////////////////////////////////////////////////////////////////////
// persistence functions

template<typename NT, typename AT>
void digraph<NT,AT>::dump(dump_context& context) const
  throw(persistent_dump_failed)
{
  // dump a magic key to the address of the graph for use in persistence of iterators
  // and register it as a dumped address
  std::pair<bool,unsigned> mapping = context.pointer_map(this);
  if (mapping.first) throw persistent_dump_failed("digraph: already dumped this graph");
  ::dump(context,mapping.second);
  // dump the nodes
  ::dump(context,size());
  for (digraph_node<NT,AT>* node = m_nodes->m_next; node != m_nodes; node = node->m_next)
  {
    // nodes are keyed by the magic key to the node address
    // this is used by iterator persistence too
    std::pair<bool,unsigned> node_mapping = context.pointer_map(node);
    if (node_mapping.first) throw persistent_dump_failed("digraph: already dumped this node");
    ::dump(context,node_mapping.second);
    // now dump the user's data
    ::dump(context,node->data);
  }  
  // dump the arcs
  ::dump(context,arc_size());
  for (digraph_arc<NT,AT>* arc = m_arcs->m_next; arc != m_arcs; arc = arc->m_next)
  {
    // dump the magic key to the arc address
    // this is used by iterator persistence too
    std::pair<bool,unsigned> arc_mapping = context.pointer_map(arc);
    if (arc_mapping.first) throw persistent_dump_failed("digraph: already dumped this arc");
    ::dump(context,arc_mapping.second);
    // now dump the from/to pointers as cross-references
    dump_xref(context,arc->m_from);
    dump_xref(context,arc->m_to);
    // now dump the user's data
    ::dump(context,arc->data);
  }
}

template<typename NT, typename AT>
void dump_digraph(dump_context& context, const digraph<NT,AT>& data)
  throw(persistent_dump_failed)
{
  data.dump(context);
}

template<typename NT, typename AT>
void digraph<NT,AT>::restore(restore_context& context)
  throw(persistent_restore_failed)
{
  clear();
  // restore the graph's magic key and map it onto the graph's address
  // this is used in the persistence of iterators
  unsigned magic = 0;
  ::restore(context,magic);
  context.pointer_add(magic,this);
  // restore the nodes
  unsigned nodes = 0;
  ::restore(context, nodes);
  for (unsigned n = 0; n < nodes; n++)
  {
    unsigned node_magic = 0;
    ::restore(context,node_magic);
    // create a new node and map the magic key onto the new address
    typename digraph<NT,AT>::iterator node = insert(NT());
    context.pointer_add(node_magic,node.m_node);
    // now restore the user's data
    ::restore(context,*node);
  }
  // restore the arcs
  unsigned arcs = 0;
  ::restore(context, arcs);
  for (unsigned a = 0; a < arcs; a++)
  {
    unsigned arc_magic = 0;
    ::restore(context,arc_magic);
    // restore the from and to cross-references
    digraph_node<NT,AT>* from = 0;
    digraph_node<NT,AT>* to = 0;
    restore_xref(context,from);
    restore_xref(context,to);
    // create an arc with these from/to pointers
    typename digraph<NT,AT>::arc_iterator arc = 
      arc_insert(TEMPORARY_TYPENAME digraph<NT,AT>::iterator(this,from), 
                 TEMPORARY_TYPENAME digraph<NT,AT>::iterator(this,to));
    context.pointer_add(arc_magic,arc.m_arc);
    // restore the user data
    ::restore(context,*arc);
  }
}

template<typename NT, typename AT>
void restore_digraph(restore_context& context, digraph<NT,AT>& data)
  throw(persistent_restore_failed)
{
  data.restore(context);
}

////////////////////////////////////////////////////////////////////////////////
