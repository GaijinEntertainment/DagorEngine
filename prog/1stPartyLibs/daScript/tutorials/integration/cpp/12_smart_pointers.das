options gen2

// Companion script for the "Smart Pointers" tutorial.
// Entity is a reference-counted C++ type (inherits ptr_ref_count).
// In daslang, smart_ptr<Entity> variables use `var inscope` to
// ensure proper cleanup when they go out of scope.

require tutorial_12_cpp

[export]
def test() {
    // --- Creating smart pointers via factory ---
    print("=== Factory-created entities ===\n")
    var inscope hero <- make_entity("Hero", 0.0, 0.0)
    var inscope enemy <- make_entity("Goblin", 10.0, 5.0)
    print("hero:  {hero.name} at ({hero.x}, {hero.y}) hp={hero.health}\n")
    print("enemy: {enemy.name} at ({enemy.x}, {enemy.y}) hp={enemy.health}\n")
    print("use_count(hero)  = {int(smart_ptr_use_count(hero))}\n")
    print("use_count(enemy) = {int(smart_ptr_use_count(enemy))}\n")

    // --- Using methods ---
    print("\n=== Moving and combat ===\n")
    move_entity(*hero, 3.0, 4.0)
    print("hero moved to ({hero.x}, {hero.y})\n")

    let dist = distance_to(*hero, *enemy)
    print("distance = {dist}\n")

    take_damage(*enemy, 30)
    print("enemy hp after 30 damage = {enemy.health}\n")
    print("enemy.is_alive = {enemy.is_alive}\n")

    take_damage(*enemy, 100)
    print("enemy hp after 100 more  = {enemy.health}\n")
    print("enemy.is_alive = {enemy.is_alive}\n")

    // --- Cloning a smart pointer (increases ref count) ---
    print("\n=== Cloning smart_ptr ===\n")
    var inscope hero2 : smart_ptr<Entity>
    smart_ptr_clone(hero2, hero)
    print("After clone:\n")
    print("  use_count(hero)  = {int(smart_ptr_use_count(hero))}\n")
    print("  use_count(hero2) = {int(smart_ptr_use_count(hero2))}\n")
    print("  hero2.name = {hero2.name}\n")

    // hero2 goes out of scope → delRef → but hero still holds a ref

    // --- Creating with `new` (canNew=true in the annotation) ---
    // `new` allocates on the heap and calls addRef().
    // No initializer arguments — just default-constructs.
    print("\n=== new Entity() ===\n")
    unsafe {
        var inscope fresh <- new Entity
        fresh.name := "Newbie"
        fresh.health = 50
        print("fresh.name   = {fresh.name}\n")
        print("fresh.health = {fresh.health}\n")
        print("use_count    = {int(smart_ptr_use_count(fresh))}\n")
    }
    // `fresh` goes out of scope → delRef → destroyed (ref_count reaches 0)

    print("\n=== End of test (hero, enemy, hero2 destroyed here) ===\n")
}
