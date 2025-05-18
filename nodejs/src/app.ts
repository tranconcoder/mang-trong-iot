import express, { type Express, type Request, type Response } from "express";
import path from "path";
import morgan from "morgan";
import { engine } from "express-handlebars";

import { connectDB } from "@/services/database.service";
import rootRoute from "@/routes";
import apiRouter from "@/routes/api.route";
import User from "./models/user.model";
import bcrypt from "bcrypt";

const app: Express = express();

// Connect MongoDB
await connectDB();
await User.findOneAndReplace(
  {
    email: "tranvanconkg@gmail.com",
  },
  {
    email: "tranvanconkg@gmail.com",
    password: bcrypt.hashSync("123", 10),
    name: "Tran Van Con",
  },
  {
    upsert: true,
    new: true,
  }
);

// Express middlewares
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use("/public", express.static(path.join(__dirname, "../public")));

// Custom middlewares
app.use(morgan("dev"));

// View engine
app.engine(
  "hbs",
  engine({
    extname: ".hbs",
    defaultLayout: "main",
    layoutsDir: path.join(__dirname, "views/layouts"),
    partialsDir: path.join(__dirname, "views/partials"),
    helpers: {
      eq: (v1: any, v2: any) => v1 === v2,
    },
  })
);
app.set("view engine", "hbs");
app.set("views", path.join(__dirname, "views"));

// Routes
app.use("/", rootRoute);

export default app;
