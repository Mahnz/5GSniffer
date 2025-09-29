/**
 * @file exceptions.h
 *
 * @brief Defines custom exceptions that should be thrown for errors specific to 
 * this project.
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <iostream>

/**
 * Generic exception for all errors related to this project. New exceptions
 * should all inherit from this class.
 */
struct sniffer_exception : public std::runtime_error {
  sniffer_exception(const std::string what_arg) : 
    std::runtime_error(what_arg) {
  }
};

/**
 * Exception used for all configuration-related errors.
 */
struct config_exception : public sniffer_exception {
  template <class T>
  config_exception(const T what_arg) : 
    sniffer_exception(std::string("Configuration exception: ") + std::string(what_arg)) {
  }
};

/**
 * Exception used for all SDR-related errors.
 */
struct sdr_exception : public sniffer_exception {
  template <class T>
  sdr_exception(const T what_arg) : 
    sniffer_exception(std::string("SDR exception: ") + std::string(what_arg)) {
  }
};

#endif
