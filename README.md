# ConsoleArrayVisualizer
Here is the target design.
## Components
* Visualizer
  * Renderer (columns, circle, ...)
  * Graphics API (Windows Console, GDI, OpenGL, DirectDraw, ...)
* Algorithms (sorts, shuffles, any algorithm that uses array)
* LibraryLoader (loads DLL)
* Utils (other stuff)
* User interface (aka. this program or `main()`)
  * RunSort (automatically shuffle, sort, check result)

## Relationship of components
Excluding Utils.

> User interface -> Visualizer, LibraryLoader, RunSort  
> LibraryLoader -> Algorithms  
> RunSort -> Algorithms, Visualizer  
> Algorithms -> Visualizer  
> Visualizer -> Renderer  
> Renderer -> Graphics API  

## How the program works
```
1. Main reads user configuration.
2. Load algorithm dll(s).
3. Initialize Visualizer.

  1. Initialize it's internals.
  2. Initialize Renderer.
  
     1. Initialize Graphics API.
     2. Initialize it's internals.
     
4. RunSort

  -------thread 1-------
  1. Handle user input.
  -------thread 2-------
  1. Run shuffle.
  2. Run sort.
  3. Run check.

5. Uninitialize Visualizer.
  1. Uninitialize Renderer.
  
     2. Uninitialize it's internals.
     1. Uninitialize Graphics API.
     
  2. Uninitialize it's internals.
6. Done.
```
