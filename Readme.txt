Here you can find a slightly enhanced version of a simple Prolog interpreter
written in C++, by Alan Mycroft.

Original source: https://www.cl.cam.ac.uk/~am21/research/funnel/prolog.c

How to build and run:

    mkdir build
    cd build
    cmake ..
    make
    ./prolog

Expected output:

    === Normal clause order:
    solve@0: app(_5,_6,cons(1,cons(2,cons(3,nil))))
      try:app(nil,_7,_7) :- true
    I = nil
    J = cons(1,cons(2,cons(3,nil)))
      try:app(cons(_8,_9),_10,cons(_8,_11)) :- app(_9,_10,_11)
        solve@1: app(_9,_6,cons(2,cons(3,nil)))
          try:app(nil,_12,_12) :- true
    I = cons(1,nil)
    J = cons(2,cons(3,nil))
          try:app(cons(_13,_14),_15,cons(_13,_16)) :- app(_14,_15,_16)
            solve@2: app(_14,_6,cons(3,nil))
              try:app(nil,_17,_17) :- true
    I = cons(1,cons(2,nil))
    J = cons(3,nil)
              try:app(cons(_18,_19),_20,cons(_18,_21)) :- app(_19,_20,_21)
                solve@3: app(_19,_6,nil)
                  try:app(nil,_22,_22) :- true
    I = cons(1,cons(2,cons(3,nil)))
    J = nil
                  try:app(cons(_23,_24),_25,cons(_23,_26)) :- app(_24,_25,_26)
                  nomatch.

    === Reversed clause order:
    solve@0: app(_5,_6,cons(1,cons(2,cons(3,nil))))
      try:app(cons(_27,_28),_29,cons(_27,_30)) :- app(_28,_29,_30)
        solve@1: app(_28,_6,cons(2,cons(3,nil)))
          try:app(cons(_31,_32),_33,cons(_31,_34)) :- app(_32,_33,_34)
            solve@2: app(_32,_6,cons(3,nil))
              try:app(cons(_35,_36),_37,cons(_35,_38)) :- app(_36,_37,_38)
                solve@3: app(_36,_6,nil)
                  try:app(cons(_39,_40),_41,cons(_39,_42)) :- app(_40,_41,_42)
                  nomatch.
                  try:app(nil,_43,_43) :- true
    I = cons(1,cons(2,cons(3,nil)))
    J = nil
              try:app(nil,_44,_44) :- true
    I = cons(1,cons(2,nil))
    J = cons(3,nil)
          try:app(nil,_45,_45) :- true
    I = cons(1,nil)
    J = cons(2,cons(3,nil))
      try:app(nil,_46,_46) :- true
    I = nil
    J = cons(1,cons(2,cons(3,nil)))
