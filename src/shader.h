#pragma once

#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> // include glad to get all the required OpenGL headers

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>


class Shader
{
public:
    // the program ID
    unsigned int ID;

    // constructor reads and builds the shader !! includes are only for fragment shader !!
    Shader(const char* vertexPath, const char* fragmentPath, std::vector<std::string> includes, int version);
    // use/activate the shader
    void use();
    // utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;

private:
    // utility function for checking shader compilation/linking errors.
	int checkCompileErrors(unsigned int shader, std::string type, 
        std::string filename, std::vector<std::string> includes);

};

#endif