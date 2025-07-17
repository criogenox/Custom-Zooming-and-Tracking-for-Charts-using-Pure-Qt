[![LinkedIn][linkedin-shield]][linkedin-url]

<!-- PROJECT LOGO -->
<div align="center">
  <a align="center">
    <img src="https://github.com/criogenox/All-in-one-Zoom-and-Tracking-for-Qt-Charts/assets/53323058/abbacd5d-0411-4a9a-8d4f-3605097c13bd.png" alt="Logo" width="500">
  </a>
  <h3 align="center">Simply focusing on data</h3>
  <p align="center">
    All-in-one custom zoom and tracking capabilities for Qt charts series 
  </p>
</div>

## About the work

<div align="justify">
  <p>
  
> While trying to create and manipulate Qt charts for another project, I realized that the functionalities I needed do not exist freely as I would like it to work. So I started making and testing some code until everything was transformed into this repository. As simple as that, and perhaps it could be helpful for another soul.

`Key Features:`

- Complete chart zooming and scrolling capabilities. 
- Label-tracking functionalities for individual series.
  
   </p>
       <p align="right">
    :muscle: don't let anyone get you down :muscle:
  </p> 
   <div>

##  General view sample

<div align="justify"> 
  <!-- <img align="right" src="https://user-images.githubusercontent.com/53323058/230650942-4c2e0ad4-2d52-46fe-aa67-8860c642e5f6.png" width="500"> -->
<img align="center" src="https://github.com/criogenox/All-in-one-Zoom-and-Tracking-for-Qt-Charts/assets/53323058/38ad6ab9-0bbf-4378-9021-eb448f6c22d3.png">
   </p>
       <p align="center">
Tracking functionality in use: series' individual labeling by crosshair.
  </p> 
</div>

## Background & Details

### Custom zoom class

<div align="justify">
  <p>

> Zooming and scrolling triggered by mouse wheel movements, press/release mouse events and keyboard toggle actions, allowing an improved data visualization and analysis of large datasets, and providing a more intuitive user experience.

**Implementation:**

Features implemented by overridden event handlers for mouse and keyboard inputs (mousePressEvent, mouseMoveEvent, mouseReleaseEvent ,mouseDoubleClickEvent, wheelEvent, keyPressEvent).

`Functionalities:`

- Wheel zoom focusing on cursor position, enhancing data's on focus experience.
- Support for horizontal panning by x-axis range adjustment (axis clipping)
- Handles double-click events to fit the chart within the view, according to the original axes range.
- Zoom-in by drag/selection rubber band area.
- Handles mouse press events to dragging and panning (warning, inverted mouse buttons).
- Restricted zoom limits/range preventing excessive zooming far beyond the available data range.
 
  </p>
   <div>

<p align="right">(<a href="#top">back to top</a>)</p>

### Tracking class

<div align="justify">
  <p>

> Two label-tracking functionalities for enhancing the interactivity of the chart by dynamic custom tooltips (labels) creation, displaying relevant data information as points are hovered or intersected by a track line. It improves data comprehension by visual associating, reducing extra needs for navigating or streamlining the data exploration process.

**Implementation:**

- Custom tooltips are created as QLabel widgets and are dynamically positioned based on the mouse cursor's vertical coordinate.

- The mouse movement events are captured by signal/slot mechanism, updating the tooltip content and positioning it accordingly.

- Label displaying duration is controlled by a timer signal/slot mechanism and tooltip visibility is managed through a proper logic flow, preventing overlapping effects and over cluttering the interface.

`Functionalities:`

- Real-Time labeling by mouse hovering or track-line intersection over the chart, reflecting the current position and data point.

- Customizable appearance: custom tooltips styled by series colors, size, borders and padding, line width, shadow effects, text font and alignment, and other Qt flags and attributes.

- Support for data and axes labels, and cursor tooltip.

- Potential support for several Series' types ([see below](#in-depth-testing-roadmap))

- Handles crosshair (continuous lines) and truncated track lines (visually emphasizing the intersection effect).

   </p>
   <div>

## Built With

<div style="display: flex; flex-direction: column; align=center">
    <img class="img"src="https://github.com/criogenox/B_ECC-Cpp-version_plot-capabilities_noGUI/assets/53323058/1fdf2d22-fb04-45aa-9db0-8bd973942914.png" alt="Logo" width="90" height="80"/>
    <img class="img"src="https://github.com/criogenox/B_ECC-Cpp-version_plot-capabilities_noGUI/assets/53323058/6870b0b2-403c-49da-b745-5714b08f4a73.png" alt="Logo" width="90" height="80"/>
    <img class="img"src="https://github.com/criogenox/B_ECC-Cpp-version_plot-capabilities_noGUI/assets/53323058/781b169a-440c-4c8a-9fbb-caa5ce150d13.png" alt="Logo" width="90" height="85"/>

### Additional info

- Tested on Windows 11, Ubuntu 22 & 24 using Qt6.7.2
  
- Extensive checking of instance deletion to ensure proper and effective resource management, preventing memory leaks.
  
- For a more in-depth understanding of the implemented method, as many comments as possible have been included.

</div>

<p align="right">(<a href="#top">back to top</a>)</p>

<!-- ROADMAP -->
## (In-depth) Testing Roadmap 

- [x] QLineSeries
- [X] QScatterSeries
- [ ] QSplineSeries

<!-- LICENSE -->
## License

Distributed under the MIT License. See [LICENSE.txt][license-url] for more information.

<!-- MARKDOWN LINKS & IMAGES -->
[linkedin-shield]: https://user-images.githubusercontent.com/53323058/230575198-fa1acbf4-8f82-4d8e-b245-3979276bc240.png
[linkedin-url]: https://www.linkedin.com/in/criogenox/
[eqsreadme-url]: https://github.com/criogenox/E-Symbolic-Railway-Dynamics-Formulation/tree/master/eqs2latex
[eqssrc-url]: https://github.com/criogenox/E-Symbolic-Railway-Dynamics-Formulation/tree/master/eqs2latex/src
[ginac-url]: https://www.ginac.de/
[license-url]: https://github.com/criogenox/All-in-one-Zoom-and-Tracking-for-Qt-Charts/tree/master?tab=MIT-1-ov-file
