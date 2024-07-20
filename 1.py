import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget, QLineEdit, QLabel, QFormLayout, QStatusBar, QTableWidget, QTableWidgetItem, QMessageBox, QTabWidget
from models import Student, Course, Teacher, Enrollment, Admin
from PyQt5.QtWidgets import QDialog, QVBoxLayout, QLineEdit, QPushButton, QLabel

class LoginDialog(QDialog):
    def __init__(self, parent=None):
        super(LoginDialog, self).__init__(parent)
        self.setWindowTitle('管理员登录')
        layout = QVBoxLayout(self)
        self.username = QLineEdit(self)
        self.username.setPlaceholderText('用户名')
        layout.addWidget(self.username)
        self.password = QLineEdit(self)
        self.password.setPlaceholderText('密码')
        self.password.setEchoMode(QLineEdit.Password)
        layout.addWidget(self.password)
        login_button = QPushButton('登录', self)
        login_button.clicked.connect(self.handle_login)
        layout.addWidget(login_button)

    def handle_login(self):
        if self.username.text() == "admin" and self.password.text() == "admin":
            self.accept()
        else:
            QMessageBox.warning(self, '错误', '用户名或密码错误')

class StudentManagementSystem(QMainWindow):
    def __init__(self):
        super().__init__()
        self.initUI()

        login = LoginDialog(self)
        if login.exec_() == QDialog.Accepted:
            self.show()
        else:
            sys.exit()

    def initUI(self):
        self.setWindowTitle('学生管理系统')
        self.setGeometry(100, 100, 800, 600)
        self.statusBar = QStatusBar()
        self.setStatusBar(self.statusBar)

        tab_widget = QTabWidget()
        self.setCentralWidget(tab_widget)

        self.student_tab = self.create_tab('Students', ['学号', '姓名', '性别', '班级'], self.add_student, self.delete_student)
        self.course_tab = self.create_tab('Courses', ['课程号', '课程名称', '学分', '教师号'], self.add_course, self.delete_course)
        self.teacher_tab = self.create_tab('Teachers', ['工号', '姓名', '系别'], self.add_teacher, self.delete_teacher)
        self.enrollment_tab = self.create_tab('Enrollments', ['学号', '课程号', '成绩'], self.add_enrollment, self.delete_enrollment)
        self.admin_tab = self.create_tab('Admins', ['管理员ID', '姓名'], self.add_admin, self.delete_admin)

        tab_widget.addTab(self.student_tab, "学生管理")
        tab_widget.addTab(self.course_tab, "课程管理")
        tab_widget.addTab(self.teacher_tab, "教师管理")
        tab_widget.addTab(self.enrollment_tab, "选课管理")
        tab_widget.addTab(self.admin_tab, "管理员管理")

        self.load_data()

    def load_data(self):
        self.update_table(self.student_tab.findChild(QTableWidget), Student.get_all())
        self.update_table(self.course_tab.findChild(QTableWidget), Course.get_all())
        self.update_table(self.teacher_tab.findChild(QTableWidget), Teacher.get_all())
        self.update_table(self.enrollment_tab.findChild(QTableWidget), Enrollment.get_all())
        self.update_table(self.admin_tab.findChild(QTableWidget), Admin.get_all())

    def create_tab(self, tab_type, headers, add_function, delete_function):
        tab = QWidget()
        layout = QVBoxLayout()
        tab.setLayout(layout)
        table = QTableWidget()
        table.setColumnCount(len(headers))
        table.setHorizontalHeaderLabels(headers)
        layout.addWidget(table)
        input_layout = self.create_input_section(headers, tab_type, add_function, table)
        layout.addLayout(input_layout)
        btn_delete = QPushButton('删除')
        btn_delete.clicked.connect(lambda: delete_function(table))
        layout.addWidget(btn_delete)
        return tab

    def create_input_section(self, labels, tab_type, add_function, table):
        form_layout = QFormLayout()
        edits = [QLineEdit() for _ in labels]
        for label, edit in zip(labels, edits):
            form_layout.addRow(QLabel(label), edit)
        btn_add = QPushButton('添加')
        btn_add.clicked.connect(lambda: add_function(edits, table))
        form_layout.addRow(btn_add)
        return form_layout

    def add_student(self, edits, table):
        student_id, name, gender, class_name = edits[0].text(), edits[1].text(), edits[2].text(), edits[3].text()
        enrolled_courses = edits[4].text().split(',') if len(edits) > 4 else []
        Student.add(student_id, name, gender, class_name, enrolled_courses)
        self.update_table(table, Student.get_all())

    def add_course(self, edits, table):
        course_id, course_name, credits, teacher_id = edits[0].text(), edits[1].text(), edits[2].text(), edits[3].text()
        Course.add(course_id, course_name, credits, teacher_id)
        self.update_table(table, Course.get_all())

    def add_teacher(self, edits, table):
        teacher_id, name, department = edits[0].text(), edits[1].text(), edits[2].text()
        taught_courses = edits[3].text().split(',') if len(edits) > 3 else []
        Teacher.add(teacher_id, name, department, taught_courses)
        self.update_table(table, Teacher.get_all())

    def add_enrollment(self, edits, table):
        student_id, course_id, grade = edits[0].text(), edits[1].text(), edits[2].text()
        Enrollment.add(student_id, course_id, grade)
        self.update_table(table, Enrollment.get_all())

    def add_admin(self, edits, table):
        admin_id, name = edits[0].text(), edits[1].text()
        Admin.add(admin_id, name)
        self.update_table(table, Admin.get_all())

    def update_table(self, table, data):
        table.setRowCount(0)
        for row_data in data:
            row_position = table.rowCount()
            table.insertRow(row_position)
            for i, value in enumerate(row_data):
                table.setItem(row_position, i, QTableWidgetItem(str(value)))

    def delete_student(self, table):
        self.delete_selected(table, Student.delete)

    def delete_course(self, table):
        self.delete_selected(table, Course.delete)

    def delete_teacher(self, table):
        self.delete_selected(table, Teacher.delete)

    def delete_enrollment(self, table):
        self.delete_selected(table, Enrollment.delete)

    def delete_admin(self, table):
        self.delete_selected(table, Admin.delete)

    def delete_selected(self, table, delete_function):
        selected_rows = table.selectionModel().selectedRows()
        if selected_rows:
            reply = QMessageBox.question(self, '确认删除', '你确定要删除选中的记录吗？', QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if reply == QMessageBox.Yes:
                for index in sorted(selected_rows, reverse=True):
                    id_to_delete = table.item(index.row(), 0).text()
                    delete_function(id_to_delete)
                    table.removeRow(index.row())

if __name__ == "__main__":
    app = QApplication(sys.argv)
    ex = StudentManagementSystem()
    sys.exit(app.exec_())
