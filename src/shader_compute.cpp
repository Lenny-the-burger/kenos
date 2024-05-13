#include "shader_compute.h"


    // constructor reads and builds the shader
ComputeShader::ComputeShader(const char* shaderPath, std::vector<std::string> includes, int version)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string shaderCode;
    std::ifstream cShaderFile;
    // ensure ifstream objects can throw exceptions:
    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        cShaderFile.open(shaderPath);
        std::stringstream cShaderStream;
        // read file's buffer contents into streams
        cShaderStream << cShaderFile.rdbuf();
        // close file handlers
        cShaderFile.close();
        // convert stream into string
        shaderCode = cShaderStream.str();

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
                shaderCode = includeStream.str() + shaderCode;
				shaderCode = "\n\n#line 1 \"" + include + "\"\n" + shaderCode;
			}
			catch (std::ifstream::failure& e)
			{
                std::cout << "ERROR::SHADER::INCLUDE_FILE_NOT_SUCCESSFULLY_READ: " << e.what() << "\n";
                std::cout << "Filename: " << include << std::endl;
			}
		}

        // Prepend version
        std::string versionString = "#version " + std::to_string(version) + "\n";
        shaderCode = versionString + shaderCode;

    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << "\n";
        std::cout << "Filename: " << shaderPath << "\n" << std::endl;
    }
    const char* cShaderCode = shaderCode.c_str();
    // 2. compile shaders
    unsigned int compute;

    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
	int shaderErrorCode = checkCompileErrors(compute, "COMPUTE", shaderPath);
    if (shaderErrorCode != 0) {
        exit(shaderErrorCode);
    }
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    shaderErrorCode = checkCompileErrors(ID, "PROGRAM", shaderPath);
	if (shaderErrorCode != 0) {
		exit(shaderErrorCode);
	}
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(compute);
}

// use/activate the shader
void ComputeShader::use()
{
    glUseProgram(ID);
}

// utility uniform functions
void ComputeShader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void ComputeShader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void ComputeShader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

// utility function for checking shader compilation/linking errors.
// ------------------------------------------------------------------------
int ComputeShader::checkCompileErrors(unsigned int shader, std::string type, std::string filename)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n";
			std::cout << "Filename: " << filename << "\n\n";
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