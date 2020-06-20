.. _viewersInteraction:

Controlling the viewer
======================

The viewer in Natron Python API can be controlled much like any other nodes:

    app1.Viewer1

Parameters in the Viewer interface inherit the :ref:`Param<Param>` class and can be
retrieved by their :ref:`script-name<autovar>` ::

    app1.Viewer1.gain.set(2)

You can then control the player, the displayed channels, the current view, the current
compositing operator, which are the input A and B,  the frame-range, the proxy level and
various other stuff with the parameters.

In GUI mode only, you can access the last viewer that was interacted with by the user::

    viewerNode = app1.getActiveViewer()

You can redraw a viewer or re-render the viewer texture by calling the following functions::


    # Refresh the viewer texture. This causes a re-evaluation of the node-graph.
    # If the second boolean parameter is set to True, the render will not attempt
    # to retrieve a texture from the cache if there is any.
    app1.refreshViewer(viewerNode, False)

    # Just redraws the OpenGL viewer. The internal texture displayed will not be
    # re-evaluated.
    app1.redrawViewer(viewerNode)
