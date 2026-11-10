struct Vec2 {
    x: float;
    y: float;

    // Overloaded addition for Vec2 and Vec3
    Vec2 can + (other: Vec2) => Vec2 {
        return Vec2 { x: this.x + other.x, y: this.y + other.y };
    }

    Vec2 can + (other: Vec3) => Vec2 {
        return Vec2 { x: this.x + other.x, y: this.y + other.y }; // Ignore z
    }
}

struct Vec3 {
    x: float;
    y: float;
    z: float;
}

struct Sprite {
    position: Vec2;
    velocity: Vec2;
    texture: unique_ptr<Texture>; // Smart pointer for resource management

    // Constructor
    constructor(pos: Vec2, vel: Vec2, tex: unique_ptr<Texture>) {
        this.position = pos;
        this.velocity = vel;
        this.texture = move(tex);
    }

    // Capability: Sprite can move
    Sprite can move() => void {
        this.position = this.position + this.velocity;
    }

    // Capability: Sprite can draw
    Sprite can draw() => void {
        // Draw texture at position (implementation omitted)
    }
}

struct Scene {
    sprites: unique_ptr<Sprite[]>; // Array of sprites
    size: size_t;
    capacity: size_t;

    constructor(initial_capacity: size_t = 16) {
        this.capacity = initial_capacity;
        this.sprites = unique_array<Sprite>(this.capacity);
        this.size = 0;
    }

    // Add a sprite to the scene
    add_sprite(sprite: Sprite) => void {
        if (this.size >= this.capacity) {
            const new_capacity = this.capacity * 2;
            new_buffer: unique_ptr<Sprite[]> = unique_array<Sprite>(new_capacity);
            if (!new_buffer) {
                return; // Allocation failure
            }
            for (i in 0...this.size - 1) {
                new_buffer[i] = move(this.sprites[i]);
            }
            this.sprites = move(new_buffer);
            this.capacity = new_capacity;
        }
        if (this.sprites) {
            this.sprites[this.size] = move(sprite);
            ++this.size;
        }
    }

    // Update all sprites that can move
    update() => void {
        if (this.sprites) {
            for (i in 0...this.size - 1) {
                if (this.sprites[i] can move) {
                    this.sprites[i].move();
                }
            }
        }
    }

    // Draw all sprites that can draw
    draw() => void {
        if (this.sprites) {
            for (i in 0...this.size - 1) {
                if (this.sprites[i] can draw) {
                    this.sprites[i].draw();
                }
            }
        }
    }

    // Operator: Add sprite using +
    Scene can + (sprite: Sprite) => void {
        this.add_sprite(sprite);
    }
}

example_usage() => void {
    // Create a scene with shared ownership
    scene: shared_ptr<Scene> = shared<Scene>(32);

    if (!scene) {
        return; // Handle null
    }

    // Create sprites with unique textures
    tex1: unique_ptr<Texture> = unique<Texture>();
    tex2: unique_ptr<Texture> = unique<Texture>();
    sprite1: Sprite = Sprite(Vec2 { x: 0.0, y: 0.0 }, Vec2 { x: 1.0, y: 1.0 }, move(tex1));
    sprite2: Sprite = Sprite(Vec2 { x: 10.0, y: 10.0 }, Vec3 { x: 2.0, y: 2.0, z: 0.0 }, move(tex2));

    // Add sprites to scene
    scene + sprite1; // Intuitive: use + to add sprite
    scene.add_sprite(sprite2); // Alternative method

    // Create another reference to the scene
    scene_ref: shared_ptr<Scene> = scene;

    // Update and draw scene
    scene_ref.update(); // Moves all sprites that can move
    scene_ref.draw();   // Draws all sprites that can draw

    // Weak reference example
    weak_ref: weak_ptr<Scene> = scene.weak();
    if (strong_ref: shared_ptr<Scene> = weak_ref.lock(); !strong_ref) {
        strong_ref.update();
    }

    // Vector operations
    v1: Vec2 = Vec2 { x: 5.0, y: 5.0 };
    v2: Vec2 = Vec2 { x: 3.0, y: 4.0 };
    v3: Vec3 = Vec3 { x: 1.0, y: 1.0, z: 0.0 };
    pos1: Vec2 = v1 + v2; // Vec2 + Vec2
    pos2: Vec2 = v1 + v3; // Vec2 + Vec3

    // Automatic cleanup when references go out of scope
}