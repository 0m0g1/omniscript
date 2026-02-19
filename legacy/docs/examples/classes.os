class Person {
    constructor(name = "person", age = 10) {
        this.name = name;
        this.age = age;
        this.fruits = ["apple", "orages"];
        this.position = {
            x: 0,
            y: 0
        }
        this.name = () => {
            console.log("hi");
        }
    }
    destructor() {
        
    }

    public hello(other = "angela") {
        console.log("hello " + this.name + " " + other);
    }

    public getFruits() {
        return this.fruits;
    }
}

let davis = new Person("davis", 20);
// // console.log(davis);
// // console.log(davis.name);
// // console.log(davis.position);
// // davis.position.y = 5;
// // console.log(davis.position.y);

// // davis.name();
// // davis.hello("Shillah");
// console.log(davis.getFruits());
// davis.fruits[1] = "banana";
// // console.log(davis.getFruits()[0])
// console.log(davis.getFruits()[1])

class Parent : public Person {
    constructor() {
        this.name = "parent";
    }
}

console.log(Person, Parent);

// const mum = new Parent();

// console.log(mum);