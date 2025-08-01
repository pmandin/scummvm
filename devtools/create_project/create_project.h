/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TOOLS_CREATE_PROJECT_H
#define TOOLS_CREATE_PROJECT_H

#ifndef __has_feature      // Optional of course.
#define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#error MSVC support requires MSVC 2015 or newer
#endif

#include <list>
#include <map>
#include <string>
#include <vector>

#include <cassert>

typedef std::list<std::string> StringList;

typedef StringList TokenList;

/**
 * Takes a given input line and creates a list of tokens out of it.
 *
 * A token in this context is separated by whitespaces. A special case
 * are quotation marks though. A string inside quotation marks is treated
 * as single token, even when it contains whitespaces.
 *
 * Thus for example the input:
 * foo bar "1 2 3 4" ScummVM
 * will create a list with the following entries:
 * "foo", "bar", "1 2 3 4", "ScummVM"
 * As you can see the quotation marks will get *removed* too.
 *
 * You can also use this with non-whitespace by passing another separator
 * character (e.g. ',').
 *
 * @param input The text to be tokenized.
 * @param separator The token separator.
 * @return A list of tokens.
 */
TokenList tokenize(const std::string &input, char separator = ' ');

/**
 * Structure to describe a game engine to be built into ScummVM.
 *
 * We do get the game engines available by parsing the "configure"
 * script of our source distribution. See "parseConfigure" for more
 * information on that.
 * @see parseConfigure
 */
struct EngineDesc {
	/**
	 * The name of the engine. We use this to determine the directory
	 * the engine is in and to create the define, which needs to be
	 * set to enable the engine.
	 */
	std::string name;

	/**
	 * A human readable description of the engine. We will use this
	 * to display a description of the engine to the user in the list
	 * of which engines are built and which are disabled.
	 */
	std::string desc;

	/**
	 * Whether the engine should be included in the build or not.
	 */
	bool enable = false;

	/**
	 * Features required for this engine.
	 */
	StringList requiredFeatures;

	/**
	 * Components wished for this engine.
	 */
	StringList wishedComponents;

	/**
	 * A list of all available sub engine names. Sub engines are engines
	 * which are built on top of an existing engines and can be only
	 * enabled when the parten engine is enabled.
	 */
	StringList subEngines;

	bool operator==(const std::string &n) const {
		return (name == n);
	}
};

typedef std::list<EngineDesc> EngineDescList;

/**
 * This function parses the project directory and creates a list of
 * available engines.
 *
 * It will also automatically setup the default build state (enabled
 * or disabled) to the state specified in the individual configure.engine
 * files.
 *
 * @param srcDir Path to the root of the project source.
 * @return List of available engines.
 */
EngineDescList parseEngines(const std::string &srcDir);

/**
 * Checks whether the specified engine is a sub engine. To determine this
 * there is a fully setup engine list needed.
 *
 * @param name Name of the engine to check.
 * @param engines List of engines.
 * @return "true", when the engine is a sub engine, "false" otherwise.
 */
bool isSubEngine(const std::string &name, const EngineDescList &engines);

/**
 * Enables or disables the specified engine in the engines list.
 *
 * This function also disables all sub engines of an engine, when it is
 * to be disabled.
 * Also this function does enable the parent of a sub engine, when a
 * sub engine is to be enabled.
 *
 * @param name Name of the engine to be enabled or disabled.
 * @param engines The list of engines, which should be operated on.
 * @param enable Whether the engine should be enabled or disabled.
 * @return "true", when it succeeded, "false" otherwise.
 */
bool setEngineBuildState(const std::string &name, EngineDescList &engines, bool enable);

/**
 * Returns a list of all defines, according to the engine list passed.
 *
 * @param features The list of engines, which should be operated on. (this may contain engines, which are *not* enabled!)
 */
StringList getEngineDefines(const EngineDescList &engines);

/**
 * Structure to define a given feature, usually an external library,
 * used to build ScummVM.
 */
struct Feature {
	const char *name;   ///< Name of the feature
	const char *define; ///< Define of the feature
	bool library; ///< Whether this feature needs to be linked to a library

	bool enable; ///< Whether the feature is enabled or not

	const char *description; ///< Human readable description of the feature

	bool operator==(const std::string &n) const {
		return (name == n);
	}
};
typedef std::list<Feature> FeatureList;

struct Component {
	std::string name;   ///< Name of the component
	std::string define; ///< Define of the component

	Feature &feature; ///< Associated feature

	std::string description; ///< Human readable description of the component

	bool needed;

	bool operator==(const std::string &n) const {
		return (name == n);
	}
};
typedef std::list<Component> ComponentList;

struct Tool {
	const char *name; ///< Name of the tools
	bool enable;      ///< Whether the tools is enabled or not
};
typedef std::list<Tool> ToolList;

/**
 * Creates a list of all features available for MSVC.
 *
 * @return A list including all features available.
 */
FeatureList getAllFeatures();

/**
 * Creates a list of all components.
 *
 * @param srcDir The source directory containing the configure script
 * @param features The features list used to link the component to its feature
 *
 * @return A list including all components available linked to its features.
 */
ComponentList getAllComponents(const std::string &srcDir, FeatureList &features);

/**
 * Disable the features for the unused components.
 *
 * @param components List of components for the build
 */
void disableComponents(const ComponentList &components);

/**
 * Returns a list of all defines, according to the feature set
 * passed.
 *
 * @param features List of features for the build (this may contain features, which are *not* enabled!)
 */
StringList getFeatureDefines(const FeatureList &features);

/**
 * Sets the state of a given feature. This can be used to
 * either include or exclude a feature.
 *
 * @param name Name of the feature.
 * @param features List of features to operate on.
 * @param enable Whether the feature should be enabled or disabled.
 * @return "true", when it succeeded, "false" otherwise.
 */
bool setFeatureBuildState(const std::string &name, FeatureList &features, bool enable);

/**
 * Gets the state of a given feature.
 *
 * @param name Name of the feature.
 * @param features List of features to operate on.
 * @return "true", when the feature is enabled, "false" otherwise.
 */
bool getFeatureBuildState(const std::string &name, const FeatureList &features);

/**
 * Specifies the required SDL version of a feature.
 */
enum SDLVersion {
	kSDLVersionAny = 0,
	kSDLVersion1, ///< SDL 1.2
	kSDLVersion2, ///< SDL 2
	kSDLVersion3  ///< SDL 3
};

/**
 * Structure to describe a build setup.
 *
 * This includes various information about which engines to
 * enable, which features should be built into the main executable.
 * It also contains the path to the project source root.
 */
struct BuildSetup {
	std::string projectName;        ///< Project name
	std::string projectDescription; ///< Project description

	std::string srcDir;     ///< Path to the sources.
	std::string filePrefix; ///< Prefix for the relative path arguments in the project files.
	std::string outputDir;  ///< Path where to put the MSVC project files.
	std::string libsDir;    ///< Path to libraries for MSVC builds.  If absent, use LIBS_DEFINE environment var instead.

	StringList includeDirs; ///< List of additional include paths
	StringList libraryDirs; ///< List of additional library paths

	EngineDescList engines; ///< Engine list for the build (this may contain engines, which are *not* enabled!).
	FeatureList features;   ///< Feature list for the build (this may contain features, which are *not* enabled!).

	ComponentList components;

	StringList defines;   ///< List of all defines for the build.
	StringList testDirs;  ///< List of all folders containing tests

	bool devTools = false;             ///< Generate project files for the tools
	bool tests = false;                ///< Generate project files for the tests
	bool runBuildEvents = false;       ///< Run build events as part of the build (generate revision number and copy engine/theme data & needed files to the build folder
	bool createInstaller = false;      ///< Create installer after the build
	SDLVersion useSDL = kSDLVersion2;  ///< Which version of SDL to use.
	bool useStaticDetection = true;    ///< Whether to link detection features inside the executable or not.
	bool useWindowsUnicode = true;     ///< Whether to use Windows Unicode APIs or ANSI APIs.
	bool useWindowsSubsystem = false;  ///< Whether to use Windows subsystem or Console subsystem (default: Console)
	bool useXCFramework = false;       ///< Whether to use Apple XCFrameworks instead of static libraries
	bool useVcpkg = false;             ///< Whether to load libraries from vcpkg or SCUMMVM_LIBS
	bool win32 = false;                ///< Target is Windows

	bool featureEnabled(const std::string &feature) const;
	Feature getFeature(const std::string &feature) const;
	const char *getSDLName() const;
};

/**
 * Quits the program with the specified error message.
 *
 * @param message The error message to print to stderr.
 */
#if defined(__GNUC__)
#define NORETURN_POST __attribute__((__noreturn__))
#elif defined(_MSC_VER)
#define NORETURN_PRE __declspec(noreturn)
#endif

#ifndef NORETURN_PRE
#define NORETURN_PRE
#endif

#ifndef NORETURN_POST
#define NORETURN_POST
#endif
void NORETURN_PRE error(const std::string &message) NORETURN_POST;

/**
 * Structure to describe a Visual Studio version specification.
 *
 * This includes various generation details for MSVC projects,
 * as well as describe the versions supported.
 */
struct MSVCVersion {
	int version;                 ///< Version number passed as parameter.
	const char *name;            ///< Full program name.
	const char *solutionFormat;  ///< Format used for solution files.
	const char *solutionVersion; ///< Version number used in solution files.
	const char *project;         ///< Version number used in project files.
	const char *toolsetMSVC;     ///< Toolset version for MSVC compiler.
	const char *toolsetLLVM;     ///< Toolset version for Clang/LLVM compiler.
};
typedef std::list<MSVCVersion> MSVCList;

enum MSVC_Architecture {
	ARCH_ARM64,
	ARCH_X86,
	ARCH_AMD64
};

std::string getMSVCArchName(MSVC_Architecture arch);
std::string getMSVCConfigName(MSVC_Architecture arch);

enum EngineDataGroup {
	kEngineDataGroupNormal,
	kEngineDataGroupCore,
	kEngineDataGroupBig,

	kEngineDataGroupCount,
};

struct EngineDataGroupResolution {
	EngineDataGroup engineDataGroup;
	const char *mkFilePath;
	const char *winHeaderPath;
};

/**
 * Creates a list of all supported versions of Visual Studio.
 *
 * @return A list including all versions available.
 */
MSVCList getAllMSVCVersions();

/**
 * Returns the definitions for a specific Visual Studio version.
 *
 * @param version The requested version.
 * @return The version information, or NULL if the version isn't supported.
 */
const MSVCVersion *getMSVCVersion(int version);

/**
 * Auto-detects the latest version of Visual Studio installed.
 *
 * @return Version number, or 0 if no installations were found.
 */
int getInstalledMSVC();

/**
 * Removes given feature from setup.
 *
 * @param setup The setup to be processed.
 * @param feature The feature to be removed
 * @return A copy of setup less feature.
 */
BuildSetup removeFeatureFromSetup(BuildSetup setup, const std::string &feature);

namespace CreateProjectTool {

/**
 * Structure for describing an FSNode. This is a very minimalistic
 * description, which includes everything we need.
 * It only contains the name of the node and whether it is a directory
 * or not.
 */
struct FSNode {
	FSNode() : name(), isDirectory(false) {}
	FSNode(const std::string &n, bool iD) : name(n), isDirectory(iD) {}

	std::string name; ///< Name of the file system node
	bool isDirectory; ///< Whether it is a directory or not
};

typedef std::list<FSNode> FileList;

/**
 * Gets a proper sequence of \t characters for the given
 * indentation level.
 *
 * For example with an indentation level of 2 this will
 * produce:
 *  \t\t
 *
 * @param indentation The indentation level
 * @return Sequence of \t characters.
 */
std::string getIndent(const int indentation);

/**
 * Converts the given path to only use backslashes.
 * This means that for example the path:
 *  foo/bar\test.txt
 * will be converted to:
 *  foo\bar\test.txt
 *
 * @param path Path string.
 * @return Converted path.
 */
std::string convertPathToWin(const std::string &path);

/**
 * Splits a file name into name and extension.
 * The file name must be only the filename, no
 * additional path name.
 *
 * @param fileName Filename to split
 * @param name Reference to a string, where to store the name.
 * @param ext Reference to a string, where to store the extension.
 */
void splitFilename(const std::string &fileName, std::string &name, std::string &ext);

/**
 * Splits a full path into directory and filename.
 * This assumes the last part is the filename, even if it
 * has no extension.
 *
 * @param path Path to split
 * @param name Reference to a string, where to store the directory part.
 * @param ext Reference to a string, where to store the filename part.
 */
void splitPath(const std::string &path, std::string &dir, std::string &file);

/**
 * Calculates the include path and PCH file path (without the base directory).
 *
 * @param filePath Path to the source file.
 * @param pchIncludeRoot Path to the PCH inclusion root directory (ending with separator).
 * @param pchDirs List of PCH directories.
 * @param pchExclude List of PCH exclusions.
 * @param separator Path separator
 * @param outPchIncludePath Output path to be used by #include directives.
 * @param outPchFilePath Output file path.
 * @param outPchFileName Output file name.
 * @return True if the file path uses PCH, false if not.
 */
bool calculatePchPaths(const std::string &sourceFilePath, const std::string &pchIncludeRoot, const StringList &pchDirs, const StringList &pchExclude, char separator, std::string &outPchIncludePath, std::string &outPchFilePath, std::string &outPchFileName);

/**
 * Returns the basename of a path.
 * examples:
 *   a/b/c/d.ext -> d.ext
 *   d.ext       -> d.ext
 *
 * @param fileName Filename
 * @return The basename
 */
std::string basename(const std::string &fileName);

/**
 * Checks whether the given file will produce an object file or not.
 *
 * @param fileName Name of the file.
 * @return "true" when it will produce a file, "false" otherwise.
 */
bool producesObjectFile(const std::string &fileName);

/**
* Convert an integer to string
*
* @param num the integer to convert
* @return string representation of the number
*/
std::string toString(int num);

/**
* Convert a string to uppercase
*
* @param str the source string
* @return The string transformed to uppercase
*/
std::string toUpper(const std::string &str);

/**
 * Returns a list of all files and directories in the specified
 * path.
 *
 * @param dir Directory which should be listed.
 * @return List of all children.
 */
FileList listDirectory(const std::string &dir);

/**
 * Create a directory at the given path.
 *
 * @param dir The path to create.
 */
void createDirectory(const std::string &dir);

/**
 * Structure representing a file tree. This contains two
 * members: name and children. "name" holds the name of
 * the node. "children" does contain all the node's children.
 * When the list "children" is empty, the node is a file entry,
 * otherwise it's a directory.
 */
struct FileNode {
	typedef std::list<FileNode *> NodeList;

	explicit FileNode(const std::string &n) : name(n), children() {}

	~FileNode() {
		for (NodeList::iterator i = children.begin(); i != children.end(); ++i)
			delete *i;
	}

	std::string name;  ///< Name of the node
	NodeList children; ///< List of children for the node
};

class ProjectProvider {
public:
	struct EngineDataGroupDef {
		StringList dataFiles;
		std::string winHeaderPath;
	};

	typedef std::map<std::string, std::string> UUIDMap;

	/**
	 * Instantiate new ProjectProvider class
	 *
	 * @param global_warnings List of warnings that apply to all projects
	 * @param project_warnings List of project-specific warnings
	 * @param version Target project version.
	 */
	ProjectProvider(StringList &global_warnings, std::map<std::string, StringList> &project_warnings, StringList &global_errors, const int version = 0);
	virtual ~ProjectProvider() {}

	/**
	 * Creates all build files
	 *
	 * @param setup Description of the desired build setup.
	 */
	void createProject(BuildSetup &setup);

	/**
	 * Returns the last path component.
	 *
	 * @param path Path string.
	 * @return Last path component.
	 */
	static std::string getLastPathComponent(const std::string &path);

protected:
	const int _version;                                  ///< Target project version
	StringList &_globalWarnings;                         ///< Global (ignored) warnings
	StringList &_globalErrors;                           ///< Global errors (promoted from warnings)
	std::map<std::string, StringList> &_projectWarnings; ///< Per-project warnings

	UUIDMap _engineUuidMap; ///< List of (project name, UUID) pairs
	UUIDMap _allProjUuidMap;

	EngineDataGroupDef _engineDataGroupDefs[kEngineDataGroupCount];

	/**
	 *  Create workspace/solution file
	 *
	 * @param setup Description of the desired build setup.
	 */
	virtual void createWorkspace(const BuildSetup &setup) = 0;

	/**
	 *  Create other files (such as build properties)
	 *
	 * @param setup Description of the desired build setup.
	 */
	virtual void createOtherBuildFiles(const BuildSetup &setup) = 0;

	/**
	 *  Add resources to the project
	 *
	 * @param setup Description of the desired build setup.
	 */
	virtual void addResourceFiles(const BuildSetup &setup, StringList &includeList, StringList &excludeList) = 0;

	/**
	 * Create a project file for the specified list of files.
	 *
	 * @param name Name of the project file.
	 * @param uuid UUID of the project file.
	 * @param setup Description of the desired build.
	 * @param moduleDir Path to the module.
	 * @param includeList Files to include (must have "moduleDir" as prefix).
	 * @param excludeList Files to exclude (must have "moduleDir" as prefix).
	 */
	virtual void createProjectFile(const std::string &name, const std::string &uuid, const BuildSetup &setup, const std::string &moduleDir,
								   const StringList &includeList, const StringList &excludeList, const std::string &pchIncludeRoot, const StringList &pchDirs, const StringList &pchExclude) = 0;

	/**
	 * Writes file entries for the specified directory node into
	 * the given project file.
	 *
	 * @param dir Directory node.
	 * @param projectFile File stream to write to.
	 * @param indentation Indentation level to use.
	 * @param objPrefix Prefix to use for object files, which would name clash.
	 * @param filePrefix Generic prefix to all files of the node.
	 */
	virtual void writeFileListToProject(const FileNode &dir, std::ostream &projectFile, const int indentation,
										const std::string &objPrefix, const std::string &filePrefix,
										const std::string &pchIncludeRoot, const StringList &pchDirs, const StringList &pchExclude) = 0;

	/**
	 * Output a list of project references to the file stream
	 *
	 * @param output File stream to write to.
	 */
	virtual void writeReferences(const BuildSetup &, std::ofstream &) {}

	/**
	 * Get the file extension for project files
	 */
	virtual const char *getProjectExtension() { return ""; }

	/**
	 * Adds files of the specified directory recursively to given project file.
	 *
	 * @param dir Path to the directory.
	 * @param projectFile Output stream object, where all data should be written to.
	 * @param includeList Files to include (must have a relative directory as prefix).
	 * @param excludeList Files to exclude (must have a relative directory as prefix).
	 * @param filePrefix Prefix to use for relative path arguments.
	 */
	void addFilesToProject(const std::string &dir, std::ostream &projectFile,
						   const StringList &includeList, const StringList &excludeList,
						   const std::string &pchIncludeRoot, const StringList &pchDirs, const StringList &pchExclude,
	                       const std::string &filePrefix);

	/**
	 * Creates a list of files of the specified module. This also
	 * creates a list of files, which should not be included.
	 * All filenames will have "moduleDir" as prefix.
	 *
	 * @param moduleDir Path to the module.
	 * @param defines List of set defines.
	 * @param testDirs List of folders containing tests.
	 * @param includeList Reference to a list, where included files should be added.
	 * @param excludeList Reference to a list, where excluded files should be added.
	 */
	void createModuleList(const std::string &moduleDir, const StringList &defines, StringList &testDirs, StringList &includeList, StringList &excludeList, StringList &pchDirs, StringList &pchExclude, bool forDetection = false) const;

	/**
	 * Creates a list of data files from a specified .mk file
	 *
	 * @param makeFilePath Path to the engine data makefile.
	 * @param defines List of set defines.
	 * @param outDataFiles Output list of data files.
	 * @param outWinHeaderPath Output Windows resource header path.
	 */
	void createDataFilesList(EngineDataGroup engineDataGroup, const std::string &baseDir, const StringList &defines, StringList &outDataFiles, std::string &outWinHeaderPath) const;

	/**
	 * Creates an UUID for every enabled engine of the
	 * passed build description.
	 *
	 * @param setup Description of the desired build.
	 * @return A map, which includes UUIDs for all enabled engines.
	 */
	UUIDMap createUUIDMap(const BuildSetup &setup) const;

	/**
	 * Creates an UUID for every enabled tool of the
	 * passed build description.
	 *
	 * @return A map, which includes UUIDs for all enabled tools.
	 */
	UUIDMap createToolsUUIDMap() const;

	/**
	 * Creates an UUID and returns it in string representation.
	 *
	 * @return A new UUID as string.
	 */
	std::string createUUID() const;

	/**
	 * Creates a name-based UUID and returns it in string representation.
	 *
	 * @param name Unique name to hash.
	 * @return A new UUID as string.
	 */
	std::string createUUID(const std::string &name) const;

private:
	/**
	 * Returns the string representation of an existing UUID.
	 *
	 * @param uuid 128-bit array.
	 * @return Existing UUID as string.
	 */
	std::string UUIDToString(unsigned char *uuid) const;

	/**
	 * This creates the engines/plugins_table.h file required for building
	 * ScummVM.
	 *
	 * @param setup Description of the desired build.
	 */
	void createEnginePluginsTable(const BuildSetup &setup);

	/**
	 * Creates resource embed files
	 */
	void createResourceEmbeds(const BuildSetup &setup) const;
};

} // namespace CreateProjectTool

#endif // TOOLS_CREATE_PROJECT_H
