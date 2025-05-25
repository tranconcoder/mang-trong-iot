import User from "@/models/user.model";
import jwtConfig from "@/configs/jwt.config";
import bcrypt from "bcrypt";
import jwt from "jsonwebtoken";

export default new (class AuthService {
  async login(email: string, password: string) {
    const user = await User.findOne({ email });
    if (!user) {
      throw new Error("User not found");
    }

    const isPasswordValid = await bcrypt.compare(password, user.password);
    if (!isPasswordValid) {
      throw new Error("Invalid password");
    }

    // Create JWT payload with user information
    const payload = {
      userId: user._id.toString(),
      email: user.email,
      name: user.name,
    };

    // Generate token with expiration
    const token = jwt.sign(payload, jwtConfig.SECRET, {
      expiresIn: jwtConfig.EXPIRES_IN || '24h'
    });

    // Return token and user information (excluding password)
    return {
      token,
      user: {
        id: user._id.toString(),
        email: user.email,
        name: user.name,
      }
    };
  }
})();
