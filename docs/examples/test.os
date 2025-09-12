extern "C" fn printf(fmt: char*, ...) => int;

struct Position {
   x: float = 10;
   y: float = 0;
   log() => void {
      printf("%.2f, %.2f", this.x as double, this.y as double);
   }
}

struct Particle {
   position = Position{5, 10};
   constructor(x: float, y: float) => void {
      this.position.x = x;
      this.position.y = y;
   }
   log() => void {
      printf("Particle(");
      this.position.log();
      printf(")");
   }
}

let p1 = Particle{100, 300};
// p1.log();
