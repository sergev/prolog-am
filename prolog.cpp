//
// A simple Prolog interpreter written in C++,
// including an example test run as main().
//
// Copyright (c) Alan Mycroft, University of Cambridge, 2000.
//
// Source: https://www.cl.cam.ac.uk/~am21/research/funnel/prolog.c
//
#include <iostream>
#include <utility>

//
// Atom is an entity, uniquely identified by a string.
//
class Atom {
    std::string atomname;

public:
    explicit Atom(std::string s) : atomname(std::move(s)) {}

    // Compare two atoms for equality.
    bool equal(Atom *t) { return (atomname == t->atomname); }

    void print() { std::cout << atomname; }
};

class Compound;

//
// Abstract interface to a Term.
//
class Term {
public:
    // Return a copy of this term.
    virtual Term *copy() = 0;

    virtual bool unify(Term *) = 0;

    virtual bool unify_compound(Compound *) = 0;

    // Print this term to cout.
    virtual void print() = 0;
};

//
// A compound term consists of an atom called a "functor" and a number
// of arguments, which are again terms.
//
class Compound : public Term {
    Atom *functor;
    int arity;
    Term **args;

public:
    // Create a compound of zero arity: f()
    explicit Compound(Atom *f) : functor(f), arity(0), args(nullptr) {}

    // Create a compound of arity one: f(a1)
    Compound(Atom *f, Term *a1) : functor(f), arity(1), args(new Term *[1])
    {
        args[0] = a1;
    };

    // Create a compound of arity two: f(a1, a2)
    Compound(Atom *f, Term *a1, Term *a2) : functor(f), arity(2), args(new Term *[2])
    {
        args[0] = a1, args[1] = a2;
    }

    // Create a compound of arity three: f(a1, a2, a3)
    Compound(Atom *f, Term *a1, Term *a2, Term *a3) : functor(f), arity(3), args(new Term *[3])
    {
        args[0] = a1, args[1] = a2, args[2] = a3;
    }

    // Print this compound to cout.
    void print() override
    {
        functor->print();
        if (arity > 0) {
            std::cout << "(";
            for (int i = 0; i < arity;) {
                args[i]->print();
                if (++i < arity)
                    std::cout << ",";
            }
            std::cout << ")";
        }
    }

    bool unify(Term *t) override { return t->unify_compound(this); }

    // Return a copy of this term.
    Term *copy() override { return copy_compound(); }

    // Return a copy of this compound.
    Compound *copy_compound() { return new Compound(this); }

private:
    // Make a copy of another compound
    explicit Compound(Compound *c)
        : functor(c->functor), arity(c->arity), args(c->arity == 0 ? nullptr : new Term *[c->arity])
    {
        for (int i = 0; i < arity; i++)
            args[i] = c->args[i]->copy();
    }

    bool unify_compound(Compound *c) override
    {
        if (!functor->equal(c->functor) || arity != c->arity)
            return false;

        for (int i = 0; i < arity; i++)
            if (!args[i]->unify(c->args[i]))
                return false;

        return true;
    }
};

//
// Variables are placeholders for arbitrary terms.
// A variable can become bound to a term via unification.
// Variables are identified by a unique index, assigned sequentially starting from 1.
//
class Variable : public Term {
    Term *instance;
    unsigned index;
    static unsigned timestamp;

public:
    Variable() : instance(this), index(++timestamp) {}

    // Unbind this variable.
    void reset() { instance = this; }

    bool unify(Term *t) override;

    // Return a copy of this term.
    Term *copy() override;

    // Print this variable to cout.
    void print() override
    {
        if (instance != this)
            instance->print();
        else
            std::cout << "_" << index;
    };

private:
    bool unify_compound(Compound *t) override { return this->unify(t); }
};

unsigned Variable::timestamp = 0;

class Program;
class VarMapping;

class Goal {
    Compound *car;
    Goal *cdr;

public:
    Goal(Compound *h, Goal *t) : car(h), cdr(t) {}

    Goal *copy()
    {
        return new Goal(car->copy_compound(), cdr ? cdr->copy() : nullptr);
    }

    Goal *append(Goal *l)
    {
        return new Goal(car, cdr ? cdr->append(l) : nullptr);
    }

    void solve(Program *p, int level, VarMapping *map);

    void print()
    {
        car->print();
        if (cdr) {
            std::cout << "; ", cdr->print();
        }
    }

    static void indent(int n)
    {
        for (int i = 0; i < n; i++)
            std::cout << "    ";
    }

};

class Clause {
public:
    Compound *head;
    Goal *body;
    Clause(Compound *h, Goal *t) : head(h), body(t) {}

    [[nodiscard]] Clause *copy() const
    {
        return new Clause(head->copy_compound(), body ? body->copy() : nullptr);
    }

    void print() const
    {
        head->print();
        std::cout << " :- ";
        if (body)
            body->print();
        else
            std::cout << "true";
    }
};

class Program {
public:
    Clause *pcar;
    Program *pcdr;
    Program(Clause *h, Program *t) : pcar(h), pcdr(t) {}
};

class Trail {
    Variable *tcar;
    Trail *tcdr;
    static Trail *sofar;
    Trail(Variable *h, Trail *t) : tcar(h), tcdr(t) {}

public:
    static Trail *Note() { return sofar; }
    static void Push(Variable *x) { sofar = new Trail(x, sofar); }

    static void Undo(Trail *whereto)
    {
        for (; sofar != whereto; sofar = sofar->tcdr)
            sofar->tcar->reset();
    }
};

Trail *Trail::sofar = nullptr;

bool Variable::unify(Term *t)
{
    if (instance != this)
        return instance->unify(t);

    Trail::Push(this);
    instance = t;
    return true;
}

Term *Variable::copy()
{
    if (instance == this) {
        Trail::Push(this);
        instance = new Variable();
    }
    return instance;
}

class VarMapping {
private:
    Variable **varvar;
    std::string *vartext;
    int size;

public:
    VarMapping(Variable *vv[], std::string vt[], int vs) : varvar(vv), vartext(vt), size(vs) {}

    void showanswer()
    {
        if (size == 0)
            std::cout << "yes\n";
        else {
            for (int i = 0; i < size; i++) {
                std::cout << vartext[i] << " = ";
                varvar[i]->print();
                std::cout << "\n";
            }
        }
    }
};

void Goal::solve(Program *p, int level, VarMapping *map)
{
    indent(level);
    std::cout << "solve@" << level << ": ";
    this->print();
    std::cout << "\n";
    for (Program *q = p; q; q = q->pcdr) {
        Trail *t = Trail::Note();
        Clause *c = q->pcar->copy();
        Trail::Undo(t);
        indent(level);
        std::cout << "  try:";
        c->print();
        std::cout << "\n";
        if (car->unify(c->head)) {
            Goal *gdash = c->body ? c->body->append(cdr) : cdr;
            if (gdash)
                gdash->solve(p, level + 1, map);
            else
                map->showanswer();
        } else {
            indent(level);
            std::cout << "  nomatch.\n";
        }
        Trail::Undo(t);
    }
}

//
// A sample test program: append
//
int main()
{
    auto *atom_app = new Atom("app");
    auto *atom_cons = new Atom("cons");
    auto *nil = new Compound(new Atom("nil"));
    auto *i_1 = new Compound(new Atom("1"));
    auto *i_2 = new Compound(new Atom("2"));
    auto *i_3 = new Compound(new Atom("3"));

    auto *var_x = new Variable();
    auto *lhs1 = new Compound(atom_app, nil, var_x, var_x);
    auto *c1 = new Clause(lhs1, nullptr);

    auto *var_l = new Variable();
    auto *var_m = new Variable();
    auto *var_n = new Variable();
    auto *rhs2 = new Compound(atom_app, var_l, var_m, var_n);
    auto *lhs2 = new Compound(atom_app, new Compound(atom_cons, var_x, var_l),
                              var_m, new Compound(atom_cons, var_x, var_n));
    auto *c2 = new Clause(lhs2, new Goal(rhs2, nullptr));

    auto *var_i = new Variable();
    auto *var_j = new Variable();
    auto *rhs3 = new Compound(atom_app, var_i, var_j,
                              new Compound(atom_cons, i_1,
                                           new Compound(atom_cons, i_2,
                                                        new Compound(atom_cons, i_3, nil))));

    auto *g1 = new Goal(rhs3, nullptr);

    auto *test_p = new Program(c1, new Program(c2, nullptr));
    auto *test_p2 = new Program(c2, new Program(c1, nullptr));

    Variable *vars[] = { var_i, var_j };
    std::string names[] = { "I", "J" };
    auto *var_name_map = new VarMapping(vars, names, 2);

    std::cout << "=======Append with normal clause order:\n";
    g1->solve(test_p, 0, var_name_map);

    std::cout << "\n=======Append with reversed normal clause order:\n";
    g1->solve(test_p2, 0, var_name_map);
    return 0;
}
