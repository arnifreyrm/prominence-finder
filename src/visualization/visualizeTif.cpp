#include <string>
#include <iostream>
#include <vtkSmartPointer.h>
#include <vtkTIFFReader.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkWarpScalar.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkImageFlip.h>

/**
 * @brief Visualizes a .tif image file using VTK.
 *
 * This function reads a .tif file specified by the given path and processes it
 * using various VTK filters and techniques to visualize the image. It corrects
 * orientation issues such as flipping and warping of the image. The visualization
 * is displayed in a render window with interactive camera controls.
 *
 * Steps involved:
 * 1. Read the .tif file using vtkTIFFReader.
 * 2. Flip the image along the Y-axis to correct upside-down issues.
 * 3. Convert image data to geometry data for 3D visualization.
 * 4. Apply a warp scalar filter to correct inverted elevations.
 * 5. Map the data to a vtkPolyDataMapper and create a vtkActor for rendering.
 * 6. Render the image in a vtkRenderWindow with interactive controls.
 *
 * @param path The file path of the .tif image to be visualized.
 */

void visualizeTif(std::string path)
{

  vtkSmartPointer<vtkTIFFReader> reader =
      vtkSmartPointer<vtkTIFFReader>::New();
  reader->SetFileName(path.c_str());
  reader->Update();

  // Flip the image data to correct upside down issue - Iceland appears upside down otherwise
  vtkSmartPointer<vtkImageFlip> flipYFilter =
      vtkSmartPointer<vtkImageFlip>::New();
  flipYFilter->SetInputConnection(reader->GetOutputPort());
  flipYFilter->SetFilteredAxis(1); // Flip along Y axis
  flipYFilter->Update();

  vtkSmartPointer<vtkImageDataGeometryFilter> geometryFilter =
      vtkSmartPointer<vtkImageDataGeometryFilter>::New();
  geometryFilter->SetInputConnection(flipYFilter->GetOutputPort());
  geometryFilter->Update();

  // Adjust warp scalar to correct inverted elevations
  vtkSmartPointer<vtkWarpScalar> warpScalar =
      vtkSmartPointer<vtkWarpScalar>::New();
  warpScalar->SetInputConnection(geometryFilter->GetOutputPort());
  warpScalar->SetScaleFactor(1);
  warpScalar->Update();

  vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(warpScalar->GetOutputPort());

  vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow =
      vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
      vtkSmartPointer<vtkRenderWindowInteractor>::New();
  interactor->SetRenderWindow(renderWindow);

  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
      vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  interactor->SetInteractorStyle(style);

  renderer->AddActor(actor);
  renderer->SetBackground(0.1, 0.2, 0.3);

  renderWindow->Render();
  interactor->Start();
};