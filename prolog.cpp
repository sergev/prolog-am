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

    // Match this term to another one, and instantiate the variables.
    virtual bool unify(Term *t) = 0;

    // Match this term to the gived compound, and instantiate the variables.
    virtual bool unify_compound(Compound *c) = 0;

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

    // Match this compound to the given term, and instantiate the variables.
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

    // Match this compound to another one, and instantiate the variables.
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
// A variable can become instantiated (bound to a term) via unification.
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

    // Match this variable to the given term, and instantiate when appropriate.
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
    // Match this variable to the given compound, and instantiate when appropriate.
    bool unify_compound(Compound *c) override { return this->unify(c); }
};

unsigned Variable::timestamp = 0;

class Program;
class VarMapping;

//
// Goal is a list of compounds:
//      a(); b(); c()
//
class Goal {
    Compound *head;
    Goal *tail;

public:
    // Create a goal with given head and tail.
    Goal(Compound *h, Goal *t = nullptr) : head(h), tail(t) {}

    // Return a copy of this goal.
    Goal *copy()
    {
        return new Goal(head->copy_compound(), tail ? tail->copy() : nullptr);
    }

    // Append a goal to this list.
    Goal *append(Goal *l)
    {
        return new Goal(head, tail ? tail->append(l) : nullptr);
    }

    // Solve the problem.
    void solve(Program *prog, int level, VarMapping *vars);

    // Print this list.
    void print()
    {
        head->print();
        if (tail) {
            std::cout << "; ", tail->print();
        }
    }

    // Print n*4 spaces.
    static void indent(int n)
    {
        for (int i = 0; i < n; i++)
            std::cout << "    ";
    }
};

//
// Clause consists of a head (compound) and a goal (list of compounds):
//      head() :- a(); b(); c().
//
class Clause {
public:
    Compound *head;
    Goal *body;
    Clause(Compound *h, Goal *t = nullptr) : head(h), body(t) {}

    // Return a copy of this clause.
    [[nodiscard]] Clause *copy() const
    {
        return new Clause(head->copy_compound(), body ? body->copy() : nullptr);
    }

    // Print this clause.
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

//
// Program is a list of clauses.
//
class Program {
public:
    Clause *head;
    Program *tail;
    Program(Clause *h, Program *t = nullptr) : head(h), tail(t) {}
};

//
// Trace records a sequence of variable instaitiations.
//
class Trace {
    Variable *head;
    Trace *tail;
    static Trace *history;
    Trace(Variable *h, Trace *t) : head(h), tail(t) {}

public:
    // Return a current position of the trace.
    static Trace *Note() { return history; }

    // Add a new variable to the trace.
    static void Push(Variable *x)
    {
        history = new Trace(x, history);
    }

    // Reset all instantiations up to the given position.
    static void Undo(Trace *whereto)
    {
        for (; history != whereto; history = history->tail)
            history->head->reset();
    }
};

Trace *Trace::history = nullptr;

//
// Bind this variable to the specified term.
//
bool Variable::unify(Term *t)
{
    if (instance == this) {
        // The variable is uninstantiated.
        // Bind it to this term.
        Trace::Push(this);
        instance = t;
        return true;
    }

    // This variable is already instantiated.
    // Redirect the request to the instance.
    return instance->unify(t);
}

//
// Return a term this variable is bound to.
//
Term *Variable::copy()
{
    if (instance == this) {
        // The variable is unbound.
        // Bind it to a newly allocated variable.
        Trace::Push(this);
        instance = new Variable();
    }
    return instance;
}

//
// Table of variables and their names.
//
class VarMapping {
private:
    Variable **vars;
    std::string *names;
    int count;

public:
    VarMapping(Variable *vv[], std::string vt[], int vs) : vars(vv), names(vt), count(vs) {}

    // Print variables and their instantiations.
    void show_answer()
    {
        if (count == 0)
            std::cout << "yes\n";
        else {
            for (int i = 0; i < count; i++) {
                std::cout << names[i] << " = ";
                vars[i]->print();
                std::cout << "\n";
            }
        }
    }
};

//
// Solve the problem.
//
void Goal::solve(Program *prog, int level, VarMapping *vars)
{
    indent(level);
    std::cout << "solve@" << level << ": ";
    this->print();
    std::cout << "\n";

    //
    // Iterate over the clauses of the program.
    //
    for (Program *iter = prog; iter; iter = iter->tail) {
        Trace *tr = Trace::Note();
        Clause *cl = iter->head->copy();
        Trace::Undo(tr);

        indent(level);
        std::cout << "  try:";
        cl->print();
        std::cout << "\n";

        // Match the goal to the clause head.
        if (head->unify(cl->head)) {

            // Matched: extend the goal with the clause body and solve it,
            // with the level incremented.
            Goal *gdash = cl->body ? cl->body->append(tail) : tail;
            if (gdash)
                gdash->solve(prog, level + 1, vars);
            else
                vars->show_answer();
        } else {
            indent(level);
            std::cout << "  nomatch.\n";
        }

        // Reset the variables bound at this iteration.
        Trace::Undo(tr);
    }
}

//
// A sample test program
//
int main()
{
    auto *atom_app = new Atom("app");
    auto *atom_cons = new Atom("cons");
    auto *nil = new Compound(new Atom("nil"));
    auto *i_1 = new Compound(new Atom("1"));
    auto *i_2 = new Compound(new Atom("2"));
    auto *i_3 = new Compound(new Atom("3"));

    //
    // Clause 1:
    //      app(nil, x, x)
    //
    auto *var_x = new Variable();
    auto *app_nil_x_x = new Compound(atom_app, nil, var_x, var_x);
    auto *clause_1 = new Clause(app_nil_x_x);

    //
    // Clause 2:
    //      app([x|l], m, [x|n]); app(l, m, n)
    //
    auto *var_l = new Variable();
    auto *var_m = new Variable();
    auto *var_n = new Variable();
    auto *app_l_m_n = new Compound(atom_app, var_l, var_m, var_n);
    auto *app_xl_m_xn = new Compound(atom_app, new Compound(atom_cons, var_x, var_l),
                                     var_m, new Compound(atom_cons, var_x, var_n));
    auto *clause_2 = new Clause(app_xl_m_xn, new Goal(app_l_m_n));

    //
    // Goal:
    //      app(i, j, [1, 2, 3])
    auto *var_i = new Variable();
    auto *var_j = new Variable();
    auto *app_i_j_123 = new Compound(atom_app, var_i, var_j,
                                     new Compound(atom_cons, i_1,
                                           new Compound(atom_cons, i_2,
                                                        new Compound(atom_cons, i_3, nil))));
    auto *goal = new Goal(app_i_j_123);

    //
    // Two test programs.
    //
    auto *prog_1 = new Program(clause_1, new Program(clause_2));
    auto *prog_2 = new Program(clause_2, new Program(clause_1));

    //
    // Two variables in the goal: I and J.
    //
    Variable *vars[] = { var_i, var_j };
    std::string names[] = { "I", "J" };
    auto *var_name_map = new VarMapping(vars, names, 2);

    //
    // Run program 1.
    //
    std::cout << "=== Normal clause order:\n";
    goal->solve(prog_1, 0, var_name_map);

    //
    // Run program 2.
    //
    std::cout << "\n=== Reversed clause order:\n";
    goal->solve(prog_2, 0, var_name_map);
    return 0;
}
