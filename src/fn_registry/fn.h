#pragma once

#include <vector>
#include <functional>
#include <any>
#include <string>
#include <tuple>
#include <stdexcept>
#include <iostream>

namespace IPC {
    /**
     * This is a class that wraps a function and allows it to be called with a vector of std::any arguments.
     * The function is called with the arguments unpacked from the vector.
     *
     * NOTE: This class is having template constructor which takes function name and function pointer as arguments.
     * there will not be any implementations written for these templates in the library so that we need to
     * make this class a header only file without a .h and .cc file combination.
     */
    class Function {
    public:
        template <typename Ret, typename... Args>
        Function(std::string name, std::function<Ret(Args...)> func) {
            /**
             * Creates an anonymous function that takes a vector of std::any arguments
             * and calls the function with the arguments unpacked from the vector.
             */
            function_ = [func](const std::vector<std::any>& args) -> std::any {
                if (args.size() != sizeof...(Args)) throw std::runtime_error("Incorrect argument count");
                auto tuple_args = vector_to_tuple<Args...>(args);
                return std::apply(func, tuple_args);
                };

            name_ = name;
            return_type_ = typeid(Ret).name();
            arg_types_ = { typeid(Args).name()... };
        }

        /**
         * Calls the function with the arguments unpacked from the vector.
         */
        std::any invoke(const std::vector<std::any>& args) {
            return function_(args);
        }

        /**
         * Prints the details of the function.
         */
        void printInfo() {
            std::cout << return_type_ << " " << name_ << "(";
            for (size_t i = 0; i < arg_types_.size(); i++) {
                std::cout << arg_types_[i];
                if (i != arg_types_.size() - 1) std::cout << ", ";
            }
            std::cout << ")" << std::endl;
        }
    private:
        template <typename... Args, std::size_t... I>
        static auto vector_to_tuple_helper(const std::vector<std::any>& vec, std::index_sequence<I...>) {
            return std::make_tuple(std::any_cast<Args>(vec[I])...);
        }

        template <typename... Args>
        static auto vector_to_tuple(const std::vector<std::any>& vec) {
            if (vec.size() != sizeof...(Args)) throw std::runtime_error("Argument count mismatch");
            return vector_to_tuple_helper<Args...>(vec, std::index_sequence_for<Args...>{});
        }

    private:
        /// Name of the function.
        std::string name_;

        /// The wrapped function.
        std::function<std::any(const std::vector<std::any>&)> function_;

        /// Return type of the function.
        std::string return_type_;

        /// Argument types of the function.
        std::vector<std::string> arg_types_;
    };
}