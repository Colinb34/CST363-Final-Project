# Quadtree Collision Demo

This is a Godot 4.6.2 project that demonstrates quadtree collision detection versus a basic collision scan.

## Install Godot 4.6.2

### Windows with winget

Open PowerShell and run:

```powershell
winget install --id GodotEngine.GodotEngine -e
```

After installation, restart PowerShell if the `godot` command is not immediately available.

### Manual install

1. Go to the Godot download page.
2. Download **Godot 4.6.2 Stable** for your operating system.
3. Extract the downloaded file if needed.
4. Run the Godot executable.

## Import The Project From A Zip

1. Download or copy the project `.zip` file.
2. Extract the zip to a normal folder first.
3. Make sure the extracted folder contains `project.godot`.
4. Open Godot 4.6.2.
5. In the Project Manager, click **Import**.
6. Browse to the extracted folder and select `project.godot`.
7. Click **Import & Edit**.

Do not import the zip file directly. Godot needs the extracted folder.

## Run The Game

Once the project is open in Godot:

1. Press `F5`, or click the **Run Project** button.
2. If Godot asks for the main scene, choose:

```text
res://scenes/Main.tscn
```

The game window should open with the map in the center, metrics on the left, and controls on the right.

## Controls

- Move: `W`, `A`, `S`, `D`
- Pause: `P`
- Change collision method: use the right-side dropdown
- Adjust forced clock speed: use the right-side slider
- Adjust enemy burst rate: use the right-side slider
- Adjust bullet speed: use the right-side slider

## What To Look For

- **Basic scan** checks every bullet against every collision object.
- **Quadtree** first filters nearby candidates, then runs exact collision checks.
- The left metrics panel shows only the currently active collision method.
- The CPU display below the map resets its average when you switch collision methods.

## Troubleshooting

If the project does not run:

- Confirm you are using **Godot 4.6.2**.
- Confirm `project.godot` is in the project root folder.
- Confirm `res://scenes/Main.tscn` exists.
- If the run button asks for a main scene, select `res://scenes/Main.tscn`.
