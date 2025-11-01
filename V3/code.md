```owly
int main(void) {
    for (int x = 0; x < 5; x++) {
        hello();
    }
    int x = 5;
    char *name = "Amity";

    ret 0;
}
void hello(void) {
    // do something, comments not supported yet btw
}
```



```
[AST]
└── Program
    ├── int main()
    |    ├── [ARGS]
    |    |   └── void
    |    ├──[BODY]
    |    |   ├── for
    |    |   |   ├── [INIT]
    |    |   |   │    └── int x
    |    |   |   |        └── 0
    |    |   |   ├── [COND]
    |    |   |   │    └── [BINOP]
    |    |   |   |         ├── [LEFT]
    |    |   |   |         |    └── x
    |    |   |   |         ├── [OPERATOR]
    |    |   |   |         |    └── <
    |    |   |   |         └── [RIGHT]
    |    |   |   |              └── 5
    |    |   |   ├── [POST]
    |    |   |   |    └── [UNOP, POST]
    |    |   |   |         ├── [ARG]
    |    |   |   |         |    └── x
    |    |   |   |         └── [OPERATOR]
    |    |   |   |              └── ++
    |    |   |   └── [BODY]
    |    |   |        └── hello()
    |    |   ├── int x
    |    |   │   └── 5
    |    |   └── char *name
    |    |       └── "Amity"
    |    └── [RET]
    |         └── 0
    └── void hello()
        ├── [ARGS]
        |   └── void
        └── [BODY]
             └── something
```


/*Control flow*/
switch case break default continue do while if else for goto
/*Property*/
auto register const inline long restrict short signed static unsigned volatile extern
/*Types*/
char double float int void _Bool _Complex _Imaginary
/*Defines and stuff*/
enum struct typedef union
/*Other*/
ret sizeof