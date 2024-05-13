#include "shader.h"


    // constructor reads and builds the shader
Shader::Shader(const char* vertexPath, const char* fragmentPath, std::vector<std::string> includes, int version)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

        // read includes
        for (int i = includes.size() - 1; i >= 0; i--)
        {
            std::string include = includes[i];
            std::ifstream includeFile;
            includeFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try
            {
                includeFile.open(include);
                std::stringstream includeStream;
                includeStream << includeFile.rdbuf();
                includeFile.close();

                // append include code to the start of the shader code
                fragmentCode = includeStream.str() + fragmentCode;
                fragmentCode = "\n\n#line 1 \"" + include + "\"\n" + fragmentCode;
            }
            catch (std::ifstream::failure& e)
            {
                std::cout << "ERROR::SHADER::INCLUDE_FILE_NOT_SUCCESSFULLY_READ: " << e.what() << "\n";
                std::cout << "Filename: " << include << std::endl;
            }
        }

        // Prepend version
        std::string versionString = "#version " + std::to_string(version) + "\n\n";
        fragmentCode = versionString + fragmentCode;
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << "\n";
        std::cout << "Filenames: " << vertexPath << ", " << fragmentPath << "\n" << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    int shaderErrorCode = checkCompileErrors(vertex, "VERTEX", vertexPath, includes);
	if (shaderErrorCode != 0)
	{
		exit(shaderErrorCode);
	}
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    shaderErrorCode = checkCompileErrors(fragment, "FRAGMENT", fragmentPath, includes);
    if (shaderErrorCode != 0)
    {
        exit(shaderErrorCode);
    }
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    shaderErrorCode = checkCompileErrors(ID, "PROGRAM", vertexPath + std::string(", ") + fragmentPath, includes);
	if (shaderErrorCode != 0)
	{
		exit(shaderErrorCode);
	}
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// use/activate the shader
void Shader::use()
{
    glUseProgram(ID);
}

// utility uniform functions
void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

// utility function for checking shader compilation/linking errors.
// ------------------------------------------------------------------------
int Shader::checkCompileErrors(unsigned int shader, std::string type, 
        std::string filename, std::vector<std::string> includes)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n\n";
            std::cout << "FILENAME: " << filename << "\n";
            std::cout << "INCLUDES:" << "\n";
			for (int i = 0; i < includes.size(); i++)
			{
				std::cout << " + " << includes[i] << "\n";
			}
			std::cout << "\n";
            std::cout << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return 1;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n";
            std::cout << "Filename: " << filename << "\n\n";
            std::cout << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return 2;
        }
    }

    return 0;
}