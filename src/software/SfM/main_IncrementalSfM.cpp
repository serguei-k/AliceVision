
// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdlib>

#include "software/SfM/SfMIncrementalEngine.hpp"

#include "third_party/cmdLine/cmdLine.h"
#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"

using namespace openMVG;

int main(int argc, char **argv)
{
  using namespace std;
  std::cout << "Incremental reconstruction" << std::endl
            << " Perform incremental SfM (Initial Pair Essential + Resection)." << std::endl
            << std::endl;

  CmdLine cmd;

  std::string sImaDirectory;
  std::string sMatchesDir;
  std::string sOutDir = "";
  bool bPmvsExport = false;
  bool bRefinePPandDisto = true;
  bool bColoredPointCloud = false;
  std::pair<size_t,size_t> initialPair(0,0);

  cmd.add( make_option('i', sImaDirectory, "imadir") );
  cmd.add( make_option('m', sMatchesDir, "matchdir") );
  cmd.add( make_option('o', sOutDir, "outdir") );
  cmd.add( make_option('p', bPmvsExport, "pmvs") );
  cmd.add( make_option('a', initialPair.first, "initialPairA") );
  cmd.add( make_option('b', initialPair.second, "initialPairB") );
  cmd.add( make_option('c', bColoredPointCloud, "coloredPointCloud") );
  cmd.add( make_option('d', bRefinePPandDisto, "refinePPandDisto") );


  try {
    if (argc == 1) throw std::string("Invalid command line parameter.");
    cmd.process(argc, argv);
  } catch(const std::string& s) {
    std::cerr << "Usage: " << argv[0] << '\n'
    << "[-i|--imadir path] \n"
    << "[-m|--matchdir path] \n"
    << "[-o|--outdir path] \n"
    << "[-p|--pmvs 0 or 1] \n"
    << "[-a|--initialPairA number] \n"
    << "[-b|--initialPairB number] \n"
    << "[-c|--coloredPointCloud 0(default) or 1]\n"
    << "[-d|--refinePPandDisto \n"
    << "\t 0-> refine only the Focal,\n"
    << "\t 1-> refine Focal, Principal point and radial distortion factors.] \n"
    << std::endl;

    std::cerr << s << std::endl;
    return EXIT_FAILURE;
  }

  if (sOutDir.empty())  {
    std::cerr << "\nIt is an invalid output directory" << std::endl;
    return EXIT_FAILURE;
  }

  if (!stlplus::folder_exists(sOutDir))
    stlplus::folder_create(sOutDir);

  //---------------------------------------
  // Incremental reconstruction process
  //---------------------------------------

  clock_t timeStart = clock();
  IncrementalReconstructionEngine to3DEngine(sImaDirectory,
                                            sMatchesDir,
                                            sOutDir,
                                            true);

  to3DEngine.setInitialPair(initialPair);
  to3DEngine.setIfRefinePrincipalPointAndRadialDisto(bRefinePPandDisto);

  if (to3DEngine.Process())
  {
    clock_t timeEnd = clock();
    std::cout << std::endl << " Ac-Sfm took : " << (timeEnd - timeStart) / CLOCKS_PER_SEC << " seconds." << std::endl;

    const reconstructorHelper & reconstructorHelperRef = to3DEngine.refToReconstructorHelper();
    std::vector<Vec3> colortracks;
    if (bColoredPointCloud)
    {
      // Compute the color of each track
      to3DEngine.ColorizeTracks(colortracks);
    }
    reconstructorHelperRef.exportToPly(
      stlplus::create_filespec(sOutDir, "FinalColorized", ".ply"),
      bColoredPointCloud ? &colortracks : NULL);

    // Export to openMVG format
    std::cout << std::endl << "Export 3D scene to openMVG format" << std::endl
      << " -- Point cloud color: " << (bColoredPointCloud ? "ON" : "OFF") << std::endl;

    reconstructorHelperRef.ExportToOpenMVGFormat(
      stlplus::folder_append_separator(sOutDir) + "SfM_output",
      to3DEngine.getFilenamesVector(),
      sImaDirectory,
      to3DEngine.getImagesSize(),
      to3DEngine.getTracks(),
      bColoredPointCloud ? &colortracks : NULL
      );

    // Manage export data to desired format
    // -> PMVS
    if (bPmvsExport)  {
      std::cout << std::endl << "Export 3D scene to PMVS format" << std::endl;
      reconstructorHelperRef.exportToPMVSFormat(
        stlplus::folder_append_separator(sOutDir) + "PMVS",
        to3DEngine.getFilenamesVector(),
        sImaDirectory);
    }

    return EXIT_SUCCESS;
  }
  else
  {
    std::cerr << "\n Something goes wrong in the Structure from Motion process" << std::endl;
  }
  return EXIT_FAILURE;
}
