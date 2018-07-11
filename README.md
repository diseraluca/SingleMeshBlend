# SingleMeshBlend

The *SingleMeshBlend* plugin registers a deformer node with maya.
The registered deformer name is **SingleBlendMesh**.

The **SingleBlendMesh** deformer transform a mesh into another.
```diff
-WARNING: The meshes should have the same number of vertices and the same vertices ID.
```

###Usage

To use the **SingleBlendMesh** deformer you have to apply it to a mesh with the following **MEL** command:
```
deformer -type SingleBlendMesh;
```
This will apply the deformer to the selected mesh. If you want to apply it to a specific mesh you can use the following command:
```
deformer -type SingleBlendMesh MeshName;
```
where *MeshName* is the name of the mesh the deformer will be applied to.

After the deformer is applied you will see a warning in the script editor. Nothing to worry about. It's the deformer telling us that no mesh is connected to its *blendMesh* attribute.
To make the warning disappear and the deformer work you have to connect the *outMesh* attribute of the **mesh shape** you want to blend to to the *blendMesh* attribute of the deformer.

The deformer is now working. You can decide how much to blend as a whole by using the deformer's *blendWeight* keyable attribute or you can decide the blend value per vertex by painting the weights of the deformer trough **Modify->Paint Attributes** tool in the maya UI.


###LICENSE
All the code in this repository is under the **MIT License**.
More informations can be found in the *LICENSE* file in the root directory of this repository.

**Copyright 2018 Luca Di Sera**
*Contacts:*
*     disera.luca@gmail.com
*      https://github.com/diseraluca
*     www.linkedin.com/in/luca-di-sera-200023167