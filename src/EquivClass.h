#ifndef CDS_EQUIV_CLASS_H
#define CDS_EQUIV_CLASS_H

#include "Common.h"
#include "DegreeSequence.h"

#include <stdexcept>

namespace CDS {

// Used for equivalence class computation.
template<typename T>
class EquivClassElement {
    T val;
    EquivClassElement *equiv;
    int deg;

public:

    EquivClassElement(const EquivClassElement &) = delete;
    EquivClassElement & operator = (const EquivClassElement &) = delete;

    EquivClassElement() { equiv = this; }

    void set_value(const T &newVal) { val = newVal; }
    T value() const { return val; }

    void set_degree(int deg_) { deg = deg_; }
    int degree() const { return deg; }

    EquivClassElement *get_class_elem() {
        EquivClassElement *final = equiv;
        while (final != final->equiv)
            final = final->equiv;

        // If updating is needed, do a full second pass and update each node of the chain.
        if (final != equiv) {
            EquivClassElement *e1 = this;
            while (e1 != e1->equiv) {
                EquivClassElement *e2 = e1->equiv;
                e1->equiv = final;
                e1 = e2;
            }
        }

        return equiv;
    }

    void update_class(EquivClassElement *elem) {
        EquivClassElement *newClass = elem->get_class_elem();
        get_class_elem()->equiv = newClass;
        equiv = newClass;
    }

};


// Track connected components
class EquivClass {

    typedef EquivClassElement<int> Element;

    const int n;      // Number of nodes
    int n_supernodes; // Number of supernodes
    int n_edges;      // Half the number of free stubs

    bool closed;      // True if the degree of a supernode dropped to zero before the construction was complete

    Element *elems;

public:

    explicit EquivClass(const DegreeSequence &ds) :
        n(ds.size()),
        n_supernodes(n)
    {
        closed = false;
        n_edges = 0;
        elems = new Element[n];
        for (int i=0; i < n; ++i) {
            deg_t d = ds[i];

            elems[i].set_value(i);
            elems[i].set_degree(d);

            n_edges += d;

            if (d == 0 && n_supernodes != 1)
                closed = true;
        }
        if (n_edges % 2 == 1)
            throw std::invalid_argument("Connectivity tracker: The degree sum must be even.");
        n_edges /= 2;
    }

    EquivClass(const EquivClass &ec) :
        n(ec.n),
        n_supernodes(ec.n_supernodes),
        n_edges(ec.n_edges),
        closed(ec.closed)
    {
        elems = new Element[n];
        for (int i=0; i < n; ++i) {
            elems[i].set_value(i);
            elems[i].update_class( &elems[ec.elems[i].get_class_elem()->value()] );
            elems[i].get_class_elem()->set_degree( ec.elems[i].get_class_elem()->degree() );
        }
    }

    ~EquivClass() { delete [] elems; }

    void connect(int a, int b) {
        n_edges--;

        auto classA = elems[a].get_class_elem();
        auto classB = elems[b].get_class_elem();

        if (classA != classB) {
            n_supernodes -= 1;

            auto degA = classA->degree();
            auto degB = classB->degree();

            elems[a].update_class(&elems[b]);

            classB->set_degree(degA + degB - 2);
        } else {
            classB->set_degree(classB->degree() - 2);
        }

        if (classB->degree() == 0 && n_edges > 0)
            closed = true;
    }

    int component_count() const { return n_supernodes; }
    int edge_count() const { return n_edges; }

    auto get_class(int u) const { return elems[u].get_class_elem(); }

    bool is_potentially_connected() const {
        return !closed && n_edges >= n_supernodes-1;
    }
};

} // namespace CDS

#endif // CDS_EQUIV_CLASS_H
