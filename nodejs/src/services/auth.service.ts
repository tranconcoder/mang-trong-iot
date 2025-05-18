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

    const token = jwt.sign({ userId: user._id }, jwtConfig.SECRET);

    return token;
  }
})();
