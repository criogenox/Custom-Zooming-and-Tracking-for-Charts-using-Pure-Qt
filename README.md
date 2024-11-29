[![LinkedIn][linkedin-shield]][linkedin-url]

<!-- PROJECT LOGO -->
<div align="center">
  <a align="center">
    <img src="https://github.com/criogenox/E-Symbolic-Railway-Dynamics-Formulation/assets/53323058/69c4827e-2319-4b96-9daf-5d5687b504ab.png" alt="Logo" width="700">
  </a>
  <h3 align="center">Simply data focusing </h3>
  <p align="center">
    All-in-one zoom and tracking capabilities for Qt charts series 
  </p>
</div>

## About the work

<div align="justify">
  <p>
  
> While trying to create and manipulate Qt charts for another project, I realized that the functionalities I needed do not exist freely as I would like it to work. So I started making and testing some code until everything was transformed into this repository. As simple as that, and perhaps it could be helpful for another soul.

`Key Features:`

- Complete chart zooming and scrolling capabilities. 
- Individual series label-tracking funtionalities.

   </p>
       <p align="right">
    :muscle: don't let anyone get you down :muscle:
  </p> 
   <div>
   
## Overview and details

### Custom zoom class

<div align="justify">
  <p>

> Zooming and scrolling triggered by mouse wheel movements, press/release mouse events and keyboard toggle actions, allowing an improved data visualization and analysis in large datasets, and providing a more intuitive user experience.

**Implementation:**

Features implemented by overridden event handlers for mouse and keyboard inputs (mousePressEvent, mouseMoveEvent, mouseReleaseEvent ,mouseDoubleClickEvent, wheelEvent, keyPressEvent).

`Functionalities:`

- Handles mouse press events to dragging and panning (warning, inverted mouse buttons).
- Support for horizontal panning by x-axis range adjustment (axis clipping)
- Wheel zoom focusing on cursor position, enhancing data's on focus experience.
- Zoom-in by drag/selection rubber band area.
- Handles double-click events to fit the chart within the view, according the origianl axes range.
- Restricted zoom limits/range preventing excessive zooming far beyond the available data range.
 
  </p>
   <div>

<p align="right">(<a href="#top">back to top</a>)</p>

### Tracking class

<div align="justify">
  <p>

> Two label-tracking functionalities for enhancing the interactivity of the chart by dynamic custom tooltips (labels) creation, displaying relevant data information as points are hovered or intersected by a track line. It improves data comprehension by visually associating, reducing extra needs for navigating or streamlining the data exploration process.

**Implementation:**

- Custom tooltips are created as QLabel widgets and are dynamically positioned based on the mouse cursor's vertical coordinate.

- The mouse movement events are captured by signal/slot mechanism updating the tooltip content and position it accordingly.

- Label displaying duration is controlled by a timer signal/slot mechanism and tooltip visibility is managed through a proper logic flow, preventing overlapping effects and over cluttering the interface.

`Functionalities:`

- Real-Time labeling by mouse hovering or track-line intersection over the chart, reflecting the current position and data point.

- Customizable appearance: custom tooltips styled by series colors, size, borders and padding, line width, shadow effects, text font and alignment, and other Qt flags and attributes.

- Support for data and axes labels, and cursor tooltip.

- Support for QLinesSeries, QSplineSeries, and QScatterSeries.

- Handles crosshair (continuous lines) and truncated track lines (visually highlighting the interception effect).

   </p>
   <div>

## Built With

<div style="display: flex; flex-direction: column; align=center">
    <img class="img"src="https://github.com/criogenox/B_ECC-Cpp-version_plot-capabilities_noGUI/assets/53323058/1fdf2d22-fb04-45aa-9db0-8bd973942914.png" alt="Logo" width="90" height="80"/>
    <img class="img"src="https://github.com/criogenox/B_ECC-Cpp-version_plot-capabilities_noGUI/assets/53323058/6870b0b2-403c-49da-b745-5714b08f4a73.png" alt="Logo" width="90" height="80"/>
    <img class="img"src="https://github.com/criogenox/B_ECC-Cpp-version_plot-capabilities_noGUI/assets/53323058/7f7c66db-97e3-49a1-92d9-df41500b54ae.png" alt="Logo" width="90" height="90"/>

### Additional info

- Tested on Ubuntu 22.04 using Qt6.7.2
  
- Extensive checking of instance deletion to ensure proper and effective resource management, preventing memory leaks.
  
- For a more in-depth understanding of the implemented method, as many comments as possible have been included.

</div>

<p align="right">(<a href="#top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the MIT License. See [LICENSE.txt][license-url] for more information.

<!-- MARKDOWN LINKS & IMAGES -->
[linkedin-shield]: https://user-images.githubusercontent.com/53323058/230575198-fa1acbf4-8f82-4d8e-b245-3979276bc240.png
[linkedin-url]: https://www.linkedin.com/in/criogenox/
[eqsreadme-url]: https://github.com/criogenox/E-Symbolic-Railway-Dynamics-Formulation/tree/master/eqs2latex
[eqssrc-url]: https://github.com/criogenox/E-Symbolic-Railway-Dynamics-Formulation/tree/master/eqs2latex/src
[ginac-url]: https://www.ginac.de/
[license-url]: https://github.com/criogenox/E-Symbolic-Railway-Dynamics-Formulation/tree/master?tab=MIT-1-ov-file
